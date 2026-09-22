# Endless Sky prototype data and artwork provenance

Source repository: https://github.com/endless-sky/endless-sky
Source paths below are relative to the repository root (`../..` from this directory).
Source revision: `902bca70c3fa8bbb57a9349c7d372cdfcf06adc4`

## Scope

The catalog includes 20 real outfits: 8 Power, 3 Guns, 3 Turrets, 3 Engines, and 3 Systems. Category names match the source data directly. Outfit descriptions and base numeric values are copied from the source data. All 21 PNGs were copied byte-for-byte from existing game assets; only their destination filenames changed. No artwork was generated or retouched. This is a representative catalog and does not assert that all of these outfits are sold at a particular planet.

## Data and unit conventions

The data files carry the copyright notice “Copyright (c) 2014 by Michael Zahniser” and GNU General Public License version 3 or later. The root `copyright` file credits Michael Zahniser and Endless Sky contributors for the general project (GPL-3+). Consult the original `copyright`, `credits.txt`, and the individual file headers for their full notices. GPL text: https://www.gnu.org/licenses/gpl-3.0.html

The source game uses frames for rates. `source/OutfitInfoDisplay.cpp:35-40` defines display scaling: energy, heat, cooling, and engine operating rates multiply source values by 60; thrust multiplies by 3,600; turning force multiplies by 60. Weapon firing loads and damage use `60 × value / reload`, matching `source/OutfitInfoDisplay.cpp:620-649`. Range for the selected simple weapons is velocity × lifetime. Values have been rounded only to remove floating-point artifacts; the Power output-per-ton display is rounded to two decimal places.

`cost` is the source base price, not a depreciation or market quote. `mass` and all space requirements are tons. `space`, `weaponSpace`, `engineSpace`, `gunPorts`, and `turretMounts` reverse the sign of the negative source capacity usage so they are positive requirements. `energy` and `heat` represent passive generation per second. `engineEnergy`/`engineHeat` represent demand during full operation, and `weaponEnergy`/`weaponHeat` represent sustained firing demand. These conditional loads are not idle demand. Missing requirements and rates are explicitly zero.

Cooling and Hyperdrive properties are supplied as contextual display attributes. A consumer that only uses the base capacity/energy/heat fields will not simulate cooling, thermal equilibrium, shield restoration, travel, projectile trajectories, or weapon firing. The Falcon image is the source ship sprite, not a rendered live configuration.

## Asset attribution

All listed images are licensed [Creative Commons Attribution-ShareAlike 4.0](https://creativecommons.org/licenses/by-sa/4.0/). Attribution below is taken from the most specific matching entry in the repository `copyright` file. This file preserves author names, derivative-work notices, source paths, and hashes so provenance can be checked.

| Prototype file | Original file | Source definition | SHA-256 |
| --- | --- | --- | --- |
| `public/assets/outfits/armageddon-core.png` | `images/outfit/core.png` | `data/human/power.txt:250` | `a05f9d4fb04c1fc73282daecd88ff9edd40f2549d6f63d508b06b741f5120fc3` |
| `public/assets/outfits/breeder-reactor.png` | `images/outfit/breeder.png` | `data/human/power.txt:226` | `f7df5e26b7ef10d2a9c7370645cfabffdd02604da48c5fe5e4efc0b8c80f8c85` |
| `public/assets/outfits/dwarf-core.png` | `images/outfit/dwarf core.png` | `data/human/power.txt:202` | `b1f8113a6efb7e582da1760f474e85abe83583a8111dad522307b6ba6a6b50cd` |
| `public/assets/outfits/fission-reactor.png` | `images/outfit/fission.png` | `data/human/power.txt:214` | `134ecc3b731da96e3320a13a608e429557aad8094ae27b687e0849619659f1b4` |
| `public/assets/outfits/fusion-reactor.png` | `images/outfit/fusion.png` | `data/human/power.txt:238` | `f9aee1213dc5228e24c1a207ddde6e9b66a689221268f120a58820e32c8d1ec2` |
| `public/assets/outfits/rt-i-radiothermal.png` | `images/outfit/small radiothermal.png` | `data/human/power.txt:165` | `24de46fc89fcecd14e2042bc0cdd3037df6eaf5880c8518f625816521ac51eba` |
| `public/assets/outfits/s3-thermionic.png` | `images/outfit/small thermionic.png` | `data/human/power.txt:177` | `1e9ca24608ac40ec58127a412b93ee51f180c3f51b90e5413b99ca693119b816` |
| `public/assets/outfits/stack-core.png` | `images/outfit/stack core.png` | `data/human/power.txt:262` | `c46f3b24e65a03ff546a93a802776ee83eda2feeeb01b79f0fe7c6032deb5189` |
| `public/assets/outfits/energy-blaster.png` | `images/outfit/blaster.png` | `data/human/weapons.txt:14` | `b58b4562841af352787b5bba992b11b3009f3da41dde4ff85d473005faa80482` |
| `public/assets/outfits/beam-laser.png` | `images/outfit/laser.png` | `data/human/weapons.txt:254` | `c9d2790368a94d3a4447312c88b0a8f6012a1b93f11d34d182e49ab0f8d19cde` |
| `public/assets/outfits/heavy-blaster.png` | `images/outfit/heavy blaster.png` | `data/human/weapons.txt:229` | `2204322f689c0863012860c4fac5bf2a24a423e8580b0792e5544c88ce0ba705` |
| `public/assets/outfits/blaster-turret.png` | `images/outfit/blaster turret.png` | `data/human/weapons.txt:75` | `f5ecd17e02a0d52f01a33b7199d268469230715dec00ca2fe9e5831a27e6eef1` |
| `public/assets/outfits/laser-turret.png` | `images/outfit/laser turret.png` | `data/human/weapons.txt:279` | `4f3ba736a67c14b8c7c3c8a4b2f4139647a8a159658ce9ed620332a3aeba5c22` |
| `public/assets/outfits/heavy-laser-turret.png` | `images/outfit/heavy laser turret.png` | `data/human/weapons.txt:343` | `4656496d8c9cec62d7377db5b9df1e1f293c6dc39c4370eee52fb033fafdf157` |
| `public/assets/outfits/x3700-ion-thruster.png` | `images/outfit/medium ion thruster.png` | `data/human/engines.txt:161` | `c6b47f89e518809ee720464f9337b8540b5f7cc37957cfa738e0e15a18d60fb5` |
| `public/assets/outfits/impala-plasma-thruster.png` | `images/outfit/medium plasma thruster.png` | `data/human/engines.txt:363` | `ab9e15694e1b85443221672e124c28007237622e8817a47179849324058ae2c2` |
| `public/assets/outfits/impala-plasma-steering.png` | `images/outfit/medium plasma steering.png` | `data/human/engines.txt:454` | `c95307abe96e202d4a51352d19f6854810bbfd3ca813424b7a5d220ea5316937` |
| `public/assets/outfits/cooling-ducts.png` | `images/outfit/cooling ducts.png` | `data/human/outfits.txt:14` | `4938eb7a9571d0fda38b04b2f4d5332fe5c7b3c5f9c4710a74b8ba8a580ab68b` |
| `public/assets/outfits/liquid-nitrogen-cooler.png` | `images/outfit/liquid nitrogen.png` | `data/human/outfits.txt:37` | `562fcb503dd64b4ed72d07c87ca9a9a0e8080851ac05b969c7187b02fc77dc7d` |
| `public/assets/outfits/hyperdrive.png` | `images/outfit/hyperdrive.png` | `data/human/outfits.txt:195` | `9ec9b5de567be4d652e6f12d847082ed4daa9b7e5681a272457fc459b44b4c34` |
| `public/assets/falcon.png` | `images/ship/falcon.png` | `data/human/ships.txt:1652` | `4cbbbb984b6f7ae717bd3017fdaa96ff748e9a20256f36368586389eb8a43807` |

### Per-image copyright notices

`public/assets/outfits/armageddon-core.png` (from `images/outfit/core.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
```

`public/assets/outfits/breeder-reactor.png` (from `images/outfit/breeder.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
Comment: Derived from works by Michael Zahniser (under the same license).
```

`public/assets/outfits/dwarf-core.png` (from `images/outfit/dwarf core.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
Comment: Derived from works by Michael Zahniser (under the same license).
```

`public/assets/outfits/fission-reactor.png` (from `images/outfit/fission.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
Comment: Derived from works by Michael Zahniser (under the same license).
```

`public/assets/outfits/fusion-reactor.png` (from `images/outfit/fusion.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
```

`public/assets/outfits/rt-i-radiothermal.png` (from `images/outfit/small radiothermal.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
Comment: Derived from works by Michael Zahniser (under the same license).
```

`public/assets/outfits/s3-thermionic.png` (from `images/outfit/small thermionic.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
Comment: Derived from works by Michael Zahniser (under the same license).
```

`public/assets/outfits/stack-core.png` (from `images/outfit/stack core.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
Comment: Derived from works by Michael Zahniser (under the same license).
```

`public/assets/outfits/energy-blaster.png` (from `images/outfit/blaster.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
Comment: Derived from works by Michael Zahniser (under the same license).
```

`public/assets/outfits/beam-laser.png` (from `images/outfit/laser.png`):

```text
Copyright: Michael Zahniser <mzahniser@gmail.com>
License: CC-BY-SA-4.0
```

`public/assets/outfits/heavy-blaster.png` (from `images/outfit/heavy blaster.png`):

```text
Copyright: 1010todd
License: CC-BY-SA-4.0
```

`public/assets/outfits/blaster-turret.png` (from `images/outfit/blaster turret.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
Comment: Derived from works by Michael Zahniser (under the same license).
```

`public/assets/outfits/laser-turret.png` (from `images/outfit/laser turret.png`):

```text
Copyright: Michael Zahniser <mzahniser@gmail.com>
License: CC-BY-SA-4.0
```

`public/assets/outfits/heavy-laser-turret.png` (from `images/outfit/heavy laser turret.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
Comment: Derived from works by Michael Zahniser (under the same license).
```

`public/assets/outfits/x3700-ion-thruster.png` (from `images/outfit/medium ion thruster.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
```

`public/assets/outfits/impala-plasma-thruster.png` (from `images/outfit/medium plasma thruster.png`):

```text
Copyright: Michael Zahniser <mzahniser@gmail.com>
License: CC-BY-SA-4.0
```

`public/assets/outfits/impala-plasma-steering.png` (from `images/outfit/medium plasma steering.png`):

```text
Copyright: Michael Zahniser <mzahniser@gmail.com>
License: CC-BY-SA-4.0
```

`public/assets/outfits/cooling-ducts.png` (from `images/outfit/cooling ducts.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
```

`public/assets/outfits/liquid-nitrogen-cooler.png` (from `images/outfit/liquid nitrogen.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
```

`public/assets/outfits/hyperdrive.png` (from `images/outfit/hyperdrive.png`):

```text
Copyright: Becca Tommaso (tommasobecca03@gmail.com)
License: CC-BY-SA-4.0
Comment: Derived from works by Michael Zahniser (under the same license).
```

`public/assets/falcon.png` (from `images/ship/falcon.png`):

```text
Copyright: Michael Zahniser <mzahniser@gmail.com>
License: CC-BY-SA-4.0
Comment: Detailed by Anarchist2.
```
