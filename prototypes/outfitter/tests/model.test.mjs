import test from 'node:test';
import assert from 'node:assert/strict';
import { createInitialState, deriveShip, fitIssue, previewTransfer, applyTransfer } from '../src/model.mjs';

const reactor = { id: 'rt-i', name: 'RT-I Radiothermal', cost: 550000, mass: 45,
  space: 45, energy: 276, heat: 612, weaponSpace: 0, engineSpace: 0,
  gunPorts: 0, turretMounts: 0 };
const small = { ...reactor, id: 'small', cost: 100000, mass: 10, space: 10, energy: 60, heat: 20 };
const catalog = [reactor, small];

function transfer(state, outfit, from, to, quantity = 1, outfits = catalog) {
  const result = applyTransfer(state, outfit, from, to, quantity, outfits);
  assert.equal(result.ok, true, result.reason);
  return result.state;
}

test('initial state is fresh and matches the explicitly illustrative baseline', () => {
  const first = createInitialState();
  const second = createInitialState();
  assert.notEqual(first.inventories.installed, second.inventories.installed);
  const ship = deriveShip(first, catalog);
  assert.equal(first.credits, 32390000);
  assert.equal(ship.outfitFree, 57);
  assert.equal(ship.mass, 993);
  assert.equal(ship.cargoFree, 90);
  assert.equal(ship.energyBalance, -86);
});

test('buying one RT-I yields a matching immutable preview and result', () => {
  const state = createInitialState();
  const snapshot = structuredClone(state);
  const preview = previewTransfer(state, reactor, 'shop', 'installed', 1, catalog);
  assert.equal(preview.ok, true);
  assert.equal(preview.cost, 550000);
  assert.equal(preview.creditDelta, -550000);
  assert.equal(preview.creditsAfter, 31840000);
  assert.equal(preview.after.outfitFree, 12);
  assert.equal(preview.after.mass, 1038);
  assert.equal(preview.after.idleEnergy, 1728);
  assert.equal(preview.after.energyBalance, 190);
  assert.equal(preview.after.generatedHeat, 3372);
  assert.deepEqual(state, snapshot);
  assert.deepEqual(transfer(state, reactor, 'shop', 'installed'), preview.stateAfter);
});

test('two RT-I units cannot fit and failure preserves exact state', () => {
  const state = createInitialState();
  const result = applyTransfer(state, reactor, 'shop', 'installed', 2, catalog);
  assert.equal(result.ok, false);
  assert.match(result.reason, /outfit space.*90.*57/);
  assert.equal(result.state, state);
});

test('purchases require funds, while moving an owned outfit is free', () => {
  const poor = { ...createInitialState(), credits: 0 };
  assert.match(applyTransfer(poor, reactor, 'shop', 'installed', 1, catalog).reason, /credits/);
  const owned = { ...poor, inventories: { ...poor.inventories, storage: { 'rt-i': 1 } } };
  const after = transfer(owned, reactor, 'storage', 'installed');
  assert.equal(after.credits, 0);
  assert.deepEqual(after.inventories.storage, {});
  assert.equal(after.inventories.installed['rt-i'], 1);
});

test('cargo purchase reserves cargo rather than installing; cargo installation charges once', () => {
  const state = createInitialState();
  const cargo = transfer(state, reactor, 'shop', 'cargo');
  const cargoShip = deriveShip(cargo, catalog);
  assert.equal(cargoShip.cargoUsed, 45);
  assert.equal(cargoShip.outfitFree, 57);
  assert.equal(cargoShip.mass, 993);
  assert.equal(cargoShip.idleEnergy, 1452);
  const installed = transfer(cargo, reactor, 'cargo', 'installed');
  assert.equal(installed.credits, cargo.credits);
  assert.equal(deriveShip(installed, catalog).cargoUsed, 0);
  assert.equal(deriveShip(installed, catalog).outfitFree, 12);
  assert.deepEqual(installed.inventories.cargo, {});
});

test('owned locations require exact stock and cannot transfer to themselves', () => {
  const state = createInitialState();
  for (const source of ['installed', 'cargo', 'storage']) {
    const result = applyTransfer(state, reactor, source, 'shop', 1, catalog);
    assert.equal(result.ok, false);
    assert.match(result.reason, /Not enough/);
    assert.equal(result.state, state);
  }
  for (const location of ['shop', 'installed', 'cargo', 'storage']) {
    assert.match(previewTransfer(state, reactor, location, location, 1, catalog).reason, /different destination/);
  }
});

test('uninstalling restores all ship stats, and selling restores base price', () => {
  const before = createInitialState();
  const installed = transfer(before, reactor, 'shop', 'installed');
  const stored = transfer(installed, reactor, 'installed', 'storage');
  assert.deepEqual(deriveShip(stored, catalog), deriveShip(before, catalog));
  assert.equal(stored.credits, installed.credits);
  const sold = transfer(stored, reactor, 'storage', 'shop');
  assert.deepEqual(sold, before);
});

test('quantities must be finite positive integers within the declared limit', () => {
  const state = createInitialState();
  for (const quantity of [0, -1, 1.5, Infinity, -Infinity, NaN, 10000, '1', undefined]) {
    const result = applyTransfer(state, reactor, 'shop', 'storage', quantity, catalog);
    assert.equal(result.ok, false, `quantity ${String(quantity)}`);
    assert.equal(result.state, state);
    assert.match(result.reason, /whole quantity/);
  }
  const free = { ...small, cost: 0 };
  assert.equal(previewTransfer(state, free, 'shop', 'storage', 9999, [free]).ok, true);
});

test('every installation capacity is enforced, and an exact fit is accepted', () => {
  const state = createInitialState();
  for (const [attribute, required, reason] of [
    ['space', 58, /outfit space/],
    ['weaponSpace', 29, /weapon space/],
    ['engineSpace', 40, /engine space/],
    ['gunPorts', 1, /gun ports/],
    ['turretMounts', 1, /turret mounts/],
  ]) {
    const outfit = { ...small, [attribute]: required };
    assert.match(fitIssue(state, outfit, 1, [outfit]), reason);
    assert.equal(previewTransfer(state, outfit, 'shop', 'installed', 1, [outfit]).ok, false);
  }
  const exact = { ...small, space: 57, weaponSpace: 28, engineSpace: 39 };
  const after = transfer(state, exact, 'shop', 'installed', 1, [exact]);
  const ship = deriveShip(after, [exact]);
  assert.equal(ship.outfitFree, 0);
  assert.equal(ship.weaponFree, 0);
  assert.equal(ship.engineFree, 0);
});

test('cargo constraints include already held stock and permit an exact fit', () => {
  const state = transfer(createInitialState(), reactor, 'shop', 'cargo', 2);
  assert.equal(deriveShip(state, catalog).cargoFree, 0);
  const result = applyTransfer(state, small, 'shop', 'cargo', 1, catalog);
  assert.equal(result.ok, false);
  assert.match(result.reason, /cargo space/);
  assert.equal(result.state, state);
  const stored = transfer(state, reactor, 'cargo', 'storage');
  assert.equal(deriveShip(stored, catalog).cargoFree, 45);
});

test('independent purchases compose and each retains the other outfit', () => {
  const state = createInitialState();
  const firstOrder = transfer(transfer(state, reactor, 'shop', 'installed'), small, 'shop', 'installed');
  const otherOrder = transfer(transfer(state, small, 'shop', 'installed'), reactor, 'shop', 'installed');
  assert.deepEqual(firstOrder, otherOrder);
  const ship = deriveShip(firstOrder, catalog);
  assert.equal(ship.outfitFree, 2);
  assert.equal(ship.idleEnergy, 1788);
  assert.equal(firstOrder.credits, 31740000);
  const removed = transfer(firstOrder, reactor, 'installed', 'shop');
  assert.equal(removed.inventories.installed.small, 1);
  assert.equal(deriveShip(removed, catalog).outfitFree, 47);
});

test('installed engine and weapon demands participate in previews and reverse', () => {
  const outfit = { ...small, engineEnergy: 30, engineHeat: 20, weaponEnergy: 40, weaponHeat: 15 };
  const before = createInitialState();
  const installed = transfer(before, outfit, 'shop', 'installed', 1, [outfit]);
  const ship = deriveShip(installed, [outfit]);
  assert.equal(ship.engineEnergy, 318);
  assert.equal(ship.weaponEnergy, 1290);
  assert.equal(ship.energyBalance, -96);
  assert.equal(ship.totalHeat, 2815);
  const removed = transfer(installed, outfit, 'installed', 'shop', 1, [outfit]);
  assert.deepEqual(deriveShip(removed, [outfit]), deriveShip(before, [outfit]));
});
