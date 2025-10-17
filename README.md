# chip8-emulator 

> C++17 + SDL2


Emulateur **Chip‑8** minimal. Rendu via SDL2 (texture 64×32), bip audio, clavier mappé standard, timers 60 Hz, wrapping des sprites, instructions compatibles.

![](https://github.com/sp4r1ng/chip8-emulator/blob/main/1.png?raw=true) 

## Fonctionnalités
- CPU ~700 Hz par défaut (configurable).
- Vidéo 64×32, wrap sur X/Y, clear instantané.
- Son
- Clavier 16 touches.
- Boucle temps réel stable


## Dépendances
- **SDL2** (headers + libs).


## Build
```bash
make
```

Binaire: `./chip8`

## Lancer
```
./chip8 <rom> [--scale N] [--hz N] [--vsync]
```

## Exemples 
```
./chip8 roms/PONG.ch8
./chip8 roms/INVADERS.ch8 --scale 14 --hz 900 --vsync
```

## Mappage Clavier 
```
1 2 3 4
Q W E R
A S D F
Z X C V
```

X -> 0x0 -> Echap
