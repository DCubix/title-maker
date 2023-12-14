from fontTools.ttLib import TTFont
import sys

icons = {}
icons_raw = {}
with TTFont(sys.argv[1], 0, ignoreDecompileErrors=True) as font:
    for x in font["cmap"].tables:
        for (code, name) in x.cmap.items():
            icons[name] = hex(code)[2:].upper()
            icons_raw[name] = code

with open('Icons.h', 'w') as fp:
    fp.write('#ifndef ICONS_H\n')
    fp.write('#define ICONS_H\n\n')
    for name in icons:
        fp.write('#define IC_%s 0x%s\n' % (name.upper(), icons[name]))
    fp.write('\n')
    fp.write('#endif\n')

    # write glyphs in a list
    fp.write('\n')
    fp.write('const uint16_t icons[] = {\n')
    for name in icons:
        fp.write('    IC_%s,\n' % name.upper())
    fp.write('};\n')
    
