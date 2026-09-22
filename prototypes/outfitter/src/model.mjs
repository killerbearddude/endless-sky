// All quantities are additions to an illustrative ship, not a real saved pilot.
const BASELINE = Object.freeze({
  name: 'Peregrine Falcon',
  mass: 993,
  outfitFree: 57,
  outfitCapacity: 540,
  weaponFree: 28,
  weaponCapacity: 240,
  engineFree: 39,
  engineCapacity: 140,
  gunPortsFree: 0,
  gunPortsTotal: 4,
  turretMountsFree: 0,
  turretMountsTotal: 4,
  cargoCapacity: 90,
  crew: 56,
  bunks: 75,
  fuel: 600,
  shields: 12800,
  hull: 3700,
  idleEnergy: 1452,
  engineEnergy: 288,
  weaponEnergy: 1250,
  generatedHeat: 2760,
  engineHeat: 0,
  weaponHeat: 0,
});

const LOCATIONS = new Set(['shop', 'installed', 'cargo', 'storage']);
const MAX_QUANTITY = 9999;
const EPSILON = 1e-8;

export function createInitialState() {
  return { credits: 32390000, inventories: { installed: {}, cargo: {}, storage: {} } };
}

function entries(outfits) {
  return outfits instanceof Map ? [...outfits.values()]
    : Array.isArray(outfits) ? outfits : Object.values(outfits ?? {});
}

function amount(outfit, attribute) {
  return outfit[attribute] ?? 0;
}

function quantityIssue(quantity) {
  return Number.isInteger(quantity) && quantity > 0 && quantity <= MAX_QUANTITY
    ? '' : 'Choose a whole quantity from 1 to 9,999.';
}

function outfitIssue(outfit) {
  if (!outfit || typeof outfit.id !== 'string' || !outfit.id)
    return 'Choose an outfit first.';
  const attributes = ['cost', 'mass', 'space', 'energy', 'heat', 'weaponSpace',
    'engineSpace', 'gunPorts', 'turretMounts', 'engineEnergy', 'engineHeat',
    'weaponEnergy', 'weaponHeat'];
  if (attributes.some(key => !Number.isFinite(amount(outfit, key))))
    return 'This outfit has incomplete specifications.';
  if (['cost', 'mass', 'space'].some(key => amount(outfit, key) < 0))
    return 'This outfit has invalid specifications.';
  return '';
}

// A catalog may be an array, a dictionary, or a Map keyed by outfit id.
export function deriveShip(state, outfits) {
  const ship = { ...BASELINE, cargoUsed: 0 };
  for (const outfit of entries(outfits)) {
    const installed = state.inventories.installed[outfit.id] ?? 0;
    ship.mass += amount(outfit, 'mass') * installed;
    ship.outfitFree -= amount(outfit, 'space') * installed;
    ship.weaponFree -= amount(outfit, 'weaponSpace') * installed;
    ship.engineFree -= amount(outfit, 'engineSpace') * installed;
    ship.gunPortsFree -= amount(outfit, 'gunPorts') * installed;
    ship.turretMountsFree -= amount(outfit, 'turretMounts') * installed;
    ship.idleEnergy += amount(outfit, 'energy') * installed;
    ship.generatedHeat += amount(outfit, 'heat') * installed;
    ship.engineEnergy += amount(outfit, 'engineEnergy') * installed;
    ship.engineHeat += amount(outfit, 'engineHeat') * installed;
    ship.weaponEnergy += amount(outfit, 'weaponEnergy') * installed;
    ship.weaponHeat += amount(outfit, 'weaponHeat') * installed;
    ship.cargoUsed += amount(outfit, 'mass') * (state.inventories.cargo[outfit.id] ?? 0);
  }
  ship.outfitUsed = ship.outfitCapacity - ship.outfitFree;
  ship.weaponUsed = ship.weaponCapacity - ship.weaponFree;
  ship.engineUsed = ship.engineCapacity - ship.engineFree;
  ship.cargoFree = ship.cargoCapacity - ship.cargoUsed;
  ship.energyBalance = ship.idleEnergy - ship.engineEnergy - ship.weaponEnergy;
  ship.idleHeat = ship.generatedHeat;
  ship.totalHeat = ship.generatedHeat + ship.engineHeat + ship.weaponHeat;
  return ship;
}

function display(number) {
  return Number(number.toFixed(3)).toLocaleString('en-US');
}

export function fitIssue(state, outfit, quantity, outfits) {
  const invalid = quantityIssue(quantity) || outfitIssue(outfit);
  if (invalid) return invalid;
  const ship = deriveShip(state, outfits);
  const constraints = [
    ['space', 'outfitFree', 'outfit space', ' t'],
    ['weaponSpace', 'weaponFree', 'weapon space', ' t'],
    ['engineSpace', 'engineFree', 'engine space', ' t'],
    ['gunPorts', 'gunPortsFree', 'gun ports', ''],
    ['turretMounts', 'turretMountsFree', 'turret mounts', ''],
  ];
  for (const [attribute, available, label, unit] of constraints) {
    const required = amount(outfit, attribute) * quantity;
    if (required > ship[available] + EPSILON)
      return `Insufficient ${label}: ${display(required)}${unit} required, ${display(ship[available])}${unit} available.`;
  }
  return '';
}

function adjustInventory(inventory, id, delta) {
  const changed = { ...inventory, [id]: (inventory[id] ?? 0) + delta };
  if (changed[id] === 0) delete changed[id];
  return changed;
}

export function previewTransfer(state, outfit, source, destination, quantity, outfits) {
  const before = deriveShip(state, outfits);
  const invalid = reason => ({ ok: false, reason, before, after: before,
    creditDelta: 0, creditsAfter: state.credits, cost: 0, quantity });
  const inputIssue = quantityIssue(quantity) || outfitIssue(outfit);
  if (inputIssue) return invalid(inputIssue);
  if (!LOCATIONS.has(source) || !LOCATIONS.has(destination))
    return invalid('Choose a valid source and destination.');
  if (source === destination)
    return invalid('Choose a different destination.');
  if (source !== 'shop' && (state.inventories[source][outfit.id] ?? 0) < quantity)
    return invalid(`Not enough in ${source}: ${state.inventories[source][outfit.id] ?? 0} available.`);

  const cost = source === 'shop' ? amount(outfit, 'cost') * quantity : 0;
  if (cost > state.credits + EPSILON)
    return invalid(`Insufficient credits: ${display(cost)} required, ${display(state.credits)} available.`);
  if (destination === 'installed') {
    const issue = fitIssue(state, outfit, quantity, outfits);
    if (issue) return invalid(issue);
  }
  if (destination === 'cargo' && amount(outfit, 'mass') * quantity > before.cargoFree + EPSILON)
    return invalid(`Insufficient cargo space: ${display(amount(outfit, 'mass') * quantity)} t required, ${display(before.cargoFree)} t available.`);

  // Selling uses the base price. Depreciation and location pricing are outside this prototype.
  const creditDelta = destination === 'shop' ? amount(outfit, 'cost') * quantity : -cost;
  const inventories = { ...state.inventories };
  if (source !== 'shop') inventories[source] = adjustInventory(inventories[source], outfit.id, -quantity);
  if (destination !== 'shop') inventories[destination] = adjustInventory(inventories[destination], outfit.id, quantity);
  const stateAfter = { ...state, credits: state.credits + creditDelta, inventories };
  const after = deriveShip(stateAfter, outfits);
  return { ok: true, reason: '', before, after, creditDelta,
    creditsAfter: stateAfter.credits, cost, quantity, stateAfter };
}

export function applyTransfer(state, outfit, source, destination, quantity, outfits) {
  const preview = previewTransfer(state, outfit, source, destination, quantity, outfits);
  return { ok: preview.ok, reason: preview.reason, state: preview.ok ? preview.stateAfter : state };
}
