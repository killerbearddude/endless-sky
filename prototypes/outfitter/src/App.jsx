import { useEffect, useMemo, useRef, useState } from 'react';
import { ArrowLeft, ArrowRight, ArrowCounterClockwise, ArrowDown, ArrowUp, MagnifyingGlass, Lightning, Crosshair, Rocket, GearSix, Target, CheckCircle, WarningCircle, CaretDown, Plus, Minus, X, Info, Package } from '@phosphor-icons/react';
import { outfits } from './catalog.js';
import { createInitialState, deriveShip, fitIssue, previewTransfer, applyTransfer } from './model.mjs';

const fmt = (n, decimals = 0) => Number(n).toLocaleString('en-US', { maximumFractionDigits: decimals });
const signed = n => `${n > 0 ? '+' : n < 0 ? '−' : ''}${fmt(Math.abs(n), 1)}`;
const categories = [{ name: 'Guns', icon: Crosshair }, { name: 'Turrets', icon: Target }, { name: 'Power', icon: Lightning }, { name: 'Engines', icon: Rocket }, { name: 'Systems', icon: GearSix }];
const locations = { shop: 'Shop', installed: 'Installed', cargo: 'Cargo', storage: 'Storage' };
const destinationNames = { installed: 'Peregrine', cargo: 'Cargo hold', storage: 'Local storage', shop: 'Shop' };
const columnDefinitions = [{ key: 'cost', label: 'Cost', unit: 'cr' }, { key: 'space', label: 'Space', unit: 't' }, { key: 'energy', label: 'Energy', unit: '/s' }, { key: 'heat', label: 'Heat', unit: '/s' }, { key: 'efficiency', label: 'Energy', unit: '/s/t' }, { key: 'fit', label: 'Fit', unit: '' }];
const allColumns = columnDefinitions.map(c => c.key);
const initialSelected = outfits.find(o => o.name === 'RT-I Radiothermal').id;
function Key({ children }) { return <kbd>{children}</kbd>; }
function Metric({ label, value, className = '' }) { return <div className={`metric ${className}`}><span>{label}</span><span className="number">{value}</span></div>; }
function Change({ label, before, after, unit = '', sign = false, tone = '' }) {
  const format = sign ? signed : fmt;
  return <div className="change-row"><span>{label}</span><span className="number">{format(before)}{unit}</span><ArrowRight size={15} /><span className={`number ${tone}`}>{format(after)}{unit}</span></div>;
}

export function App() {
  const [state, setState] = useState(createInitialState);
  const [category, setCategory] = useState('Power');
  const [source, setSource] = useState('shop');
  const [destination, setDestination] = useState('installed');
  const [selectedId, setSelectedId] = useState(initialSelected);
  const [search, setSearch] = useState('');
  const [fitsOnly, setFitsOnly] = useState(false);
  const [quantity, setQuantity] = useState('1');
  const [columns, setColumns] = useState(allColumns);
  const [columnsOpen, setColumnsOpen] = useState(false);
  const [sort, setSort] = useState({ key: '', ascending: true });
  const [notice, setNotice] = useState(null);
  const [history, setHistory] = useState([]);
  const [helpOpen, setHelpOpen] = useState(false);
  const [leaveOpen, setLeaveOpen] = useState(false);
  const searchRef = useRef(null), catalogRef = useRef(null), columnsRef = useRef(null), dialogRef = useRef(null);
  const ship = useMemo(() => deriveShip(state, outfits), [state]);
  const qty = Number(quantity), validQty = Number.isInteger(qty) && qty > 0 && qty <= 9999;
  const selected = outfits.find(o => o.id === selectedId);
  const shownColumns = columnDefinitions.filter(c => columns.includes(c.key));
  const visible = useMemo(() => {
    const list = outfits.filter(o => o.category === category && o.name.toLowerCase().includes(search.toLowerCase()) &&
      (source === 'shop' || (state.inventories[source][o.id] || 0) > 0) && (!fitsOnly || !fitIssue(state, o, validQty ? qty : 1, outfits)));
    if (sort.key) list.sort((a, b) => {
      const value = o => sort.key === 'name' ? o.name : sort.key === 'efficiency' ? o.energy / o.space : sort.key === 'fit' ? (fitIssue(state, o, validQty ? qty : 1, outfits) ? 1 : 0) : sort.key === 'energy' ? o.energy || o.engineEnergy || o.weaponEnergy || 0 : sort.key === 'heat' ? o.heat || o.engineHeat || o.weaponHeat || 0 : o[sort.key];
      const av = value(a), bv = value(b);
      return (typeof av === 'string' ? av.localeCompare(bv) : av - bv) * (sort.ascending ? 1 : -1);
    });
    return list;
  }, [state, category, source, search, fitsOnly, qty, validQty, sort]);
  useEffect(() => { if (visible.length && !visible.some(o => o.id === selectedId)) setSelectedId(visible[0].id); }, [visible, selectedId]);
  const hasSelection = visible.some(o => o.id === selectedId);
  const preview = selected && hasSelection ? previewTransfer(state, selected, source, destination, qty, outfits) : null;
  const after = preview?.ok ? preview.after : ship;
  const ownedCount = location => selected ? state.inventories[location][selected.id] || 0 : 0;
  const total = selected && hasSelection && validQty ? selected.cost * qty : 0;
  const isPower = category === 'Power';
  function changeSource(next) { setFitsOnly(false); setSource(next); setDestination(next === 'installed' ? 'storage' : 'installed'); setNotice(null); }
  function toggleSort(key) { setSort(old => ({ key, ascending: old.key === key ? !old.ascending : true })); }
  function transaction(target = destination) {
    if (!selected || !hasSelection) return;
    const result = applyTransfer(state, selected, source, target, qty, outfits);
    if (!result.ok) { setNotice({ text: result.reason, error: true }); return; }
    setHistory(old => [...old, state]); setState(result.state);
    const verb = target === 'shop' ? 'Sold' : source === 'shop' ? 'Purchased' : target === 'installed' ? 'Installed' : 'Moved';
    setNotice({ text: `${verb} ${qty} × ${selected.name}${target !== 'shop' ? ` → ${destinationNames[target]}` : ''}.`, error: false });
  }
  function undo() { if (!history.length) return; setState(history[history.length - 1]); setHistory(old => old.slice(0, -1)); setNotice({ text: 'Last transaction undone.', error: false }); }
  function reset() {
    setState(createInitialState()); setHistory([]); setCategory('Power'); setSource('shop'); setDestination('installed'); setSelectedId(initialSelected); setSearch(''); setFitsOnly(false); setQuantity('1'); setSort({ key: '', ascending: true }); setColumns(allColumns); setNotice(null); setLeaveOpen(false); setHelpOpen(false);
  }
  useEffect(() => { if (!notice) return; const timer = setTimeout(() => setNotice(null), 6000); return () => clearTimeout(timer); }, [notice]);
  useEffect(() => {
    function clickOutside(event) { if (columnsRef.current && !columnsRef.current.contains(event.target)) setColumnsOpen(false); }
    document.addEventListener('pointerdown', clickOutside); return () => document.removeEventListener('pointerdown', clickOutside);
  }, []);
  useEffect(() => {
    function onKey(event) {
      if ((helpOpen || leaveOpen) && event.key === 'Tab') {
        const controls = [...dialogRef.current.querySelectorAll('button')];
        if (event.shiftKey && document.activeElement === controls[0]) { event.preventDefault(); controls.at(-1).focus(); }
        else if (!event.shiftKey && document.activeElement === controls.at(-1)) { event.preventDefault(); controls[0].focus(); }
        return;
      }
      if (event.key === 'Escape') {
        if (helpOpen || leaveOpen) { setHelpOpen(false); setLeaveOpen(false); return; }
        if (columnsOpen) { setColumnsOpen(false); return; }
        if (search) { setSearch(''); searchRef.current?.blur(); } else setLeaveOpen(true);
        return;
      }
      if (['INPUT', 'TEXTAREA', 'SELECT'].includes(event.target.tagName) || event.ctrlKey || event.metaKey || event.altKey || leaveOpen || helpOpen) return;
      if (event.key.toLowerCase() === 'f') { event.preventDefault(); searchRef.current?.focus(); }
      if (event.key.toLowerCase() === 'b') { event.preventDefault(); transaction(); }
      if ((event.key === 'ArrowDown' || event.key === 'ArrowUp') && visible.length) {
        event.preventDefault();
        const next = Math.max(0, Math.min(visible.length - 1, visible.findIndex(o => o.id === selectedId) + (event.key === 'ArrowDown' ? 1 : -1)));
        setSelectedId(visible[next].id);
        requestAnimationFrame(() => { const row = catalogRef.current?.querySelector(`[data-outfit="${visible[next].id}"]`); row?.scrollIntoView({ block: 'nearest' }); row?.querySelector('button')?.focus({ preventScroll: true }); });
      }
    }
    window.addEventListener('keydown', onKey); return () => window.removeEventListener('keydown', onKey);
  });
  function fitLabel(outfit) {
    const q = validQty ? qty : 1;
    if (outfit.space * q > ship.outfitFree) return `Needs ${fmt(outfit.space * q - ship.outfitFree)} t`;
    const issue = fitIssue(state, outfit, q, outfits);
    return issue ? outfit.gunPorts > ship.gunPortsFree ? 'No gun ports' : outfit.turretMounts > ship.turretMountsFree ? 'No mounts' : 'Does not fit' : 'Fits';
  }
  function columnValue(outfit, key) {
    if (key === 'fit') return fitLabel(outfit);
    if (key === 'efficiency') return outfit.energy ? (outfit.energy / outfit.space).toFixed(2) : '—';
    if (key === 'energy') return fmt(outfit.energy || outfit.engineEnergy || outfit.weaponEnergy || 0, 1);
    if (key === 'heat') return fmt(outfit.heat || outfit.engineHeat || outfit.weaponHeat || 0, 1);
    return fmt(outfit[key], 1);
  }
  const actionLabel = source === 'shop' ? destination === 'installed' ? 'Buy & install' : destination === 'cargo' ? 'Buy to cargo' : 'Buy to storage' : destination === 'installed' ? 'Install' : destination === 'shop' ? 'Sell' : source === 'installed' ? 'Uninstall' : 'Move outfit';
  const alternateDestination = source === 'shop' ? (destination === 'cargo' ? 'installed' : 'cargo') : 'shop';

  const actionDisabled = !hasSelection || !preview?.ok;
  const tableStyle = { gridTemplateColumns: `minmax(180px, 2.35fr) ${shownColumns.map(c => c.key === 'cost' || c.key === 'fit' ? 'minmax(90px,1fr)' : 'minmax(68px,.8fr)').join(' ')}` };

  return <div className="app-shell">
    <header className="topbar"><div className="wordmark">ENDLESS SKY</div><nav aria-label="Port services">{['Port', 'Trade', 'Outfitter', 'Shipyard'].map(name => <button key={name} className={`service ${name === 'Outfitter' ? 'active' : ''}`} aria-current={name === 'Outfitter' ? 'page' : undefined} onClick={() => name !== 'Outfitter' && setLeaveOpen(true)}>{name}</button>)}</nav><div className="port-context"><span>Port / Outfitter</span><small>New Boston</small></div><button className="back-button" onClick={() => setLeaveOpen(true)}><ArrowLeft size={23} /><span>Back</span><span className="key-inline">[Esc]</span></button></header>
    <main className="outfitter-layout">
      <aside className="category-rail" aria-label="Outfit categories"><div className="rail-title"><h1>OUTFITTER</h1><p>Command Deck · Equip for what’s next.</p></div><nav>{categories.map(({ name, icon: Icon }) => <button key={name} className={`category-button ${category === name ? 'selected' : ''}`} aria-pressed={category === name} onClick={() => { setCategory(name); setSearch(''); setSort({ key: '', ascending: true }); }}><Icon size={30} weight="fill" /><span>{name}</span></button>)}</nav><div className="selected-ship"><p>Selected ship</p><img src="/assets/falcon.png" alt="Falcon spacecraft" /><strong>Peregrine</strong><span>Falcon class</span><small>1 ship selected</small></div></aside>
      <section className="workspace" aria-label="Outfit catalog and preview">
        <div className="catalog-toolbar"><label className="search-box"><MagnifyingGlass size={24} /><input ref={searchRef} aria-label="Search outfits" placeholder="Search outfits…" value={search} onChange={e => setSearch(e.target.value)} />{search ? <button aria-label="Clear search" onClick={() => setSearch('')}><X size={17} /></button> : <Key>F</Key>}</label><div className="source-tabs" role="tablist" aria-label="Inventory location">{Object.entries(locations).map(([value, name]) => <button key={value} role="tab" aria-selected={source === value} className={source === value ? 'active' : ''} onClick={() => changeSource(value)}>{name}</button>)}</div><label className="fits-toggle"><input type="checkbox" checked={fitsOnly} onChange={e => setFitsOnly(e.target.checked)} /><span>Fits selected ship</span></label><div className="columns-control" ref={columnsRef}><button className="outline-button" aria-expanded={columnsOpen} onClick={() => setColumnsOpen(v => !v)}>Columns <CaretDown size={15} /></button>{columnsOpen && <div className="columns-popover"><strong>Visible columns</strong>{columnDefinitions.map(c => <label key={c.key}><input type="checkbox" checked={columns.includes(c.key)} onChange={() => setColumns(old => old.includes(c.key) ? old.filter(k => k !== c.key) : [...old, c.key])} />{c.label} {c.unit}</label>)}<button onClick={() => setColumns(allColumns)}>Restore all columns</button></div>}</div></div>
        <div className="table-meta"><span>{source !== 'shop' ? `${locations[source]} · ${visible.length} catalog ${visible.length === 1 ? 'item' : 'items'}` : search || fitsOnly ? `${visible.length} matching outfits` : ''}</span><span>Fit based on {fmt(ship.outfitFree)} t free{qty > 1 ? ` · quantity ${fmt(qty)}` : ''}</span></div>
        <div className="catalog-container" ref={catalogRef}><table className="catalog-table" aria-label={`${category} outfits`}><thead><tr style={tableStyle}><th aria-sort={sort.key === 'name' ? sort.ascending ? 'ascending' : 'descending' : 'none'}><button onClick={() => toggleSort('name')}>Outfit {sort.key === 'name' && (sort.ascending ? <ArrowUp size={12} /> : <ArrowDown size={12} />)}</button></th>{shownColumns.map(c => <th key={c.key} aria-sort={sort.key === c.key ? sort.ascending ? 'ascending' : 'descending' : 'none'}><button onClick={() => toggleSort(c.key)}>{c.label}{!isPower && c.key === 'energy' ? ' draw' : ''}<span>{c.unit}</span>{sort.key === c.key && (sort.ascending ? <ArrowUp size={12} /> : <ArrowDown size={12} />)}</button></th>)}</tr></thead><tbody>{visible.map(outfit => <tr key={outfit.id} style={tableStyle} data-outfit={outfit.id} className={selectedId === outfit.id ? 'selected-row' : ''} onClick={() => setSelectedId(outfit.id)}><td><button className="outfit-select" aria-label={`Select ${outfit.name}`} aria-pressed={selectedId === outfit.id}><img src={outfit.image} alt="" /><span>{outfit.name}</span></button></td>{shownColumns.map(c => <td key={c.key} className={`number ${c.key === 'fit' ? `fit-cell ${fitIssue(state, outfit, validQty ? qty : 1, outfits) ? 'warning' : 'positive'}` : ''}`}>{columnValue(outfit, c.key)}</td>)}</tr>)}</tbody></table>{!visible.length && <div className="empty-state"><Package size={40} weight="light" /><h2>{search || fitsOnly ? 'No outfits match' : `No catalog items in ${source === 'installed' ? 'your installed inventory' : locations[source].toLowerCase()}`}</h2><p>{search || fitsOnly ? 'Try a different search or show outfits that do not fit.' : 'Purchase or move equipment here to see it in this view.'}</p><button className="outline-button" onClick={() => { setSearch(''); setFitsOnly(false); changeSource('shop'); }}>Browse all {category.toLowerCase()}</button></div>}</div>
        {hasSelection && selected ? <section className="item-detail" aria-label="Selected outfit details"><div className="specification"><h2>{selected.name}</h2><div className="specification-body"><img className="detail-art" src={selected.image} alt={selected.name} /><div className="specification-copy"><p className="outfit-description" title={selected.description}>{selected.description}</p><Metric label="Mass" value={`${fmt(selected.mass)} t`} /><Metric label="Outfit space" value={`${fmt(selected.space)} t`} />{isPower ? <><Metric label="Energy output" value={`${fmt(selected.energy)} /s`} /><Metric label="Heat generation" value={`${fmt(selected.heat)} /s`} /><Metric label="Output per ton" value={`${(selected.energy / selected.space).toFixed(2)} /s/t`} /><Metric label="Thermal ratio" value={`${(selected.energy / selected.heat).toFixed(3)} energy/heat`} /></> : selected.attributes?.slice(0, 4).map(a => <Metric key={a.label} label={a.label} value={a.value} />)}</div></div><div className="ownership">Installed <b>{ownedCount('installed')}</b><span>·</span>Cargo <b>{ownedCount('cargo')}</b><span>·</span>Local storage <b>{ownedCount('storage')}</b></div></div><div className="preview"><div className="preview-heading"><h2>{destination === 'installed' ? 'Installation' : destination === 'cargo' ? 'Cargo' : destination === 'shop' ? 'Sale' : 'Transfer'} preview</h2><span>Current <ArrowRight size={15} /> After</span></div><Change label="Outfit space free" before={ship.outfitFree} after={after.outfitFree} unit=" t" /><Change label="Ship mass" before={ship.mass} after={after.mass} unit=" t" /><Change label="Net idle energy /s" before={ship.idleEnergy} after={after.idleEnergy} sign tone={after.idleEnergy > 0 ? 'positive' : 'warning'} /><Change label="Thrust + fire net /s" before={ship.energyBalance} after={after.energyBalance} sign tone={after.energyBalance >= 0 ? 'positive' : 'warning'} /><Metric className="heat-delta" label="Generated heat /s" value={`${signed(after.generatedHeat - ship.generatedHeat)} (change)`} />{destination === 'cargo' && <Change label="Cargo space free" before={ship.cargoFree} after={after.cargoFree} unit=" t" />}<p className="preview-note">{preview?.ok ? <>Energy scenario excludes repair.<br />Extra heat is not a temperature prediction.</> : <span className="warning">{preview?.reason}</span>}</p></div></section> : <div className="detail-empty">Select an outfit to inspect its specifications and preview a transfer.</div>}
      </section>
      <aside className="ship-inspector" aria-label="Ship statistics and transaction"><div className="balance">Balance <span className="number">{fmt(state.credits)} cr</span></div><div className="inspector-surface"><div className="ship-stat-scroll"><div className="ship-heading"><img src="/assets/falcon.png" alt="" /><div><h2>Peregrine · Falcon</h2><p>Current ship</p><h3>Durability</h3><Metric label="Shields" value={fmt(ship.shields)} /><Metric label="Hull" value={fmt(ship.hull)} /></div></div><div className="stat-group"><h3>Capacity</h3><Metric label="Outfit space free" value={`${fmt(ship.outfitFree)} / ${ship.outfitCapacity} t`} /><Metric label="Weapon space free" value={`${fmt(ship.weaponFree)} / ${ship.weaponCapacity} t`} /><Metric label="Engine space free" value={`${fmt(ship.engineFree)} / ${ship.engineCapacity} t`} /><Metric label="Gun ports free" value={`${ship.gunPortsFree} / ${ship.gunPortsTotal}`} /><Metric label="Turret mounts free" value={`${ship.turretMountsFree} / ${ship.turretMountsTotal}`} /></div><div className="stat-group"><h3>Ship</h3><Metric label="Cargo capacity" value={`${ship.cargoCapacity} t`} />{ship.cargoUsed > 0 && <Metric label="Cargo used" value={`${fmt(ship.cargoUsed)} t`} />}<Metric label="Crew / bunks" value={`${ship.crew} / ${ship.bunks}`} /><Metric label="Fuel capacity" value={ship.fuel} /><Metric label="Mass" value={`${fmt(ship.mass)} t`} /></div></div>
        <div className="transaction"><div className="quantity-row"><label htmlFor="quantity">Quantity</label><div className="quantity-input"><button aria-label="Decrease quantity" disabled={!validQty || qty <= 1} onClick={() => setQuantity(String(Math.max(1, qty - 1)))}><Minus size={18} /></button><input id="quantity" inputMode="numeric" type="number" min="1" max="9999" step="1" value={quantity} onChange={e => setQuantity(e.target.value)} /><button aria-label="Increase quantity" disabled={qty >= 9999} onClick={() => setQuantity(String(Math.min(9999, validQty ? qty + 1 : 1)))}><Plus size={18} /></button></div></div><div className="transfer-route"><label htmlFor="source">Source</label><select id="source" value={source} onChange={e => changeSource(e.target.value)}>{Object.entries(locations).map(([value, name]) => <option key={value} value={value}>{name}</option>)}</select><ArrowRight size={16} /><select aria-label="Destination" value={destination} onChange={e => setDestination(e.target.value)}>{Object.entries(destinationNames).filter(([value]) => value !== source).map(([value, name]) => <option key={value} value={value}>{name}</option>)}</select></div><div className="transaction-status" aria-live="polite">{!hasSelection ? <p className="muted"><Info size={20} />Select an outfit</p> : preview?.ok ? <><p className="positive"><CheckCircle size={21} weight="fill" />{destination === 'installed' ? `Fits: ${fmt(after.outfitFree)} t remain` : destination === 'cargo' ? `${fmt(after.cargoFree)} t cargo space remain` : 'Ready to transfer'}</p>{after.generatedHeat > ship.generatedHeat && <p className="warning"><WarningCircle size={21} weight="fill" />Adds {fmt(after.generatedHeat - ship.generatedHeat)} heat/s</p>}</> : <p className="warning"><WarningCircle size={21} weight="fill" />{preview?.reason}</p>}</div><div className="credit-summary"><Metric label="Credits" value={`${fmt(state.credits)} cr`} /><Metric label={destination === 'shop' ? 'Sale value' : source === 'shop' ? 'Purchase' : 'Transfer cost'} value={`${fmt(source === 'shop' || destination === 'shop' ? total : 0)} cr`} /><Metric className="credit-after" label="After transfer" value={`${fmt(preview?.ok ? preview.creditsAfter : state.credits)} cr`} /></div><div className="action-buttons"><button className="primary-button" disabled={actionDisabled} onClick={() => transaction()}>{actionLabel}<span>[B]</span></button><button className="outline-button secondary-action" disabled={!hasSelection} title="Select destination and review the transfer before committing" onClick={() => setDestination(alternateDestination)}>{source === 'shop' ? alternateDestination === 'installed' ? 'Preview fit' : 'To cargo' : 'Preview sale'}</button></div></div></div></aside>
    </main>
    <footer className="bottom-bar"><div className="key-hints"><span><Key>F</Key>Search</span><span><Key>B</Key>{actionLabel}</span><span><Key>Esc</Key>Back</span></div><div className="session-actions"><button disabled={!history.length} onClick={undo}><ArrowCounterClockwise size={17} />Undo</button><button onClick={reset}>Reset session</button><button className="prototype-note" onClick={() => setHelpOpen(true)}><Info size={17} />Real outfit data · illustrative ship state</button></div></footer>
    {notice && <div className={`toast ${notice.error ? 'error' : ''}`} role="status">{notice.error ? <WarningCircle size={22} /> : <CheckCircle size={22} />}<span>{notice.text}</span><button aria-label="Dismiss notification" onClick={() => setNotice(null)}><X size={18} /></button></div>}
    {(helpOpen || leaveOpen) && <div className="dialog-backdrop" onClick={() => { setHelpOpen(false); setLeaveOpen(false); }}><section ref={dialogRef} className="dialog" role="dialog" aria-modal="true" aria-labelledby="dialog-title" onClick={e => e.stopPropagation()}><button className="dialog-close" aria-label="Close dialog" autoFocus onClick={() => { setHelpOpen(false); setLeaveOpen(false); }}><X size={24} /></button><p className="eyebrow">ENDLESS SKY · DESIGN PROTOTYPE</p><h2 id="dialog-title">{helpOpen ? 'A fitting room for the new interface.' : 'You’re in the Outfitter prototype.'}</h2><p>This working concept uses original game artwork and real equipment specifications. Peregrine’s starting configuration is illustrative, and transactions stay in this session.</p><p>{helpOpen ? 'The preview models direct capacity and energy changes. Repair, cooling efficiency, temperature, licenses, depreciation, and full flight simulation are outside this prototype. Buy and sell prices use base catalog cost.' : 'Port, Trade, and Shipyard are context for this screen. Continue fitting your ship, or reset the session to try a different configuration.'}</p><div className="dialog-actions"><button className="outline-button" onClick={reset}>Reset session</button><button className="primary-button" onClick={() => { setHelpOpen(false); setLeaveOpen(false); }}>Continue outfitting</button></div></section></div>}
  </div>;
}
