# KiCad Files

Open in KiCad 8 or 9 (free at kicad.org).

## Files

- `moto_audio_unit_rev2.kicad_sch` — Current schematic (Rev 2.0)
- `moto_audio_unit_rev1.kicad_sch` — Previous schematic (Rev 1.0, archived)

## First Time Opening

1. Open KiCad → Open Existing Project → select the .kicad_sch file
2. All symbols are embedded in the file (no external lib needed)
3. Run Tools → Update Schematic from Library to resolve standard footprints
4. For BM62 footprint: install Microchip KiCad library via Plugin Manager

## Next Steps

1. Review schematic, run ERC (Electrical Rules Check)
2. Create PCB layout (.kicad_pcb) from schematic netlist
3. Export Gerbers for JLCPCB
