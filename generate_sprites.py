#!/usr/bin/env python3
from PIL import Image, ImageDraw
import os

TILE_SIZE = 64

os.makedirs('resources/tiles', exist_ok=True)

if not os.path.exists('resources/tiles/grass.png'):
    grass = Image.new('RGBA', (TILE_SIZE, TILE_SIZE), (40, 100, 40, 255))
    draw = ImageDraw.Draw(grass)
    for _ in range(30):
        x = _ * 7 % TILE_SIZE
        y = (_ * 13) % TILE_SIZE
        shade = 30 + (_ % 30)
        draw.ellipse([x, y, x+4, y+6], fill=(shade, shade + 30, shade, 255))
    draw.rectangle([0, 0, TILE_SIZE-1, TILE_SIZE-1], outline=(20, 80, 20, 255), width=2)
    grass.save('resources/tiles/grass.png')
    print('grass.png criado!')
else:
    print('grass.png ja existe, pulando...')

if not os.path.exists('resources/tiles/water.png'):
    water = Image.new('RGBA', (TILE_SIZE, TILE_SIZE), (20, 60, 100, 255))
    draw = ImageDraw.Draw(water)
    for i in range(3):
        y = 15 + i * 18
        draw.arc([5, y, TILE_SIZE-5, y+12], 0, 180, fill=(60, 100, 140, 255), width=3)
    water.save('resources/tiles/water.png')
    print('water.png criado!')
else:
    print('water.png ja existe, pulando...')

if not os.path.exists('resources/tiles/sand.png'):
    sand = Image.new('RGBA', (TILE_SIZE, TILE_SIZE), (210, 190, 140, 255))
    draw = ImageDraw.Draw(sand)
    for _ in range(20):
        x = _ * 11 % (TILE_SIZE - 4)
        y = (_ * 7) % (TILE_SIZE - 2)
        draw.ellipse([x, y, x+3, y+2], fill=(190, 170, 120, 255))
    sand.save('resources/tiles/sand.png')
    print('sand.png criado!')
else:
    print('sand.png ja existe, pulando...')

if not os.path.exists('resources/tiles/earth.png'):
    earth = Image.new('RGBA', (TILE_SIZE, TILE_SIZE), (80, 60, 40, 255))
    draw = ImageDraw.Draw(earth)
    for _ in range(25):
        x = _ * 9 % (TILE_SIZE - 3)
        y = (_ * 11) % (TILE_SIZE - 3)
        draw.ellipse([x, y, x+3, y+3], fill=(60, 45, 30, 255))
    earth.save('resources/tiles/earth.png')
    print('earth.png criado!')
else:
    print('earth.png ja existe, pulando...')

if not os.path.exists('resources/tiles/tree.png'):
    tree = Image.new('RGBA', (TILE_SIZE, TILE_SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(tree)
    draw.polygon([(32, 5), (8, 55), (56, 55)], fill=(20, 60, 20, 255))
    draw.polygon([(32, 15), (14, 50), (50, 50)], fill=(25, 75, 25, 255))
    draw.rectangle([28, 48, 36, 64], fill=(60, 40, 20, 255))
    tree.save('resources/tiles/tree.png')
    print('tree.png criado!')
else:
    print('tree.png ja existe, pulando...')

print('\nSprites verificados em resources/tiles/')
