# Evaluation Bug Investigation 2025-09-10



The move that SeaJay chooses when playing as black here is terrible. A strong external engine consensus is that f5a5 is better, and there are a number of other more quiet moves that are "suitable" but not losing, such as g6h7, g6h6, f5a5, f5d5, etc.. but SeaJay choose the exchange, and as we can see in the subsequent analysis, this is a terrible choice for black.

There are a number of questionable moves in these games, most denoted by the symbol/ string "??".


## Example Game 1
### Before the mistake

Script output from tools/analyze_position.sh

Four-Engine Position Analysis

FEN: 8/5p2/2R2Pk1/5r1p/5P1P/5KP1/8/8 b - - 26 82
Side to move: Black
Search mode: depth = 14
Temp directory: /tmp/chess_analysis_20250901_124112_97557

----------------------------------------


Engine Results Summary


Scores (as reported by engines):

SeaJay:         -2.38  White -1.50-3.00  
Stash:          -0.68  White -0.50-1.50  
Komodo:         -0.91  White -0.50-1.50  
Laser:          -1.02  White -0.50-1.50  

Best Moves:
SeaJay:      f5f6     (depth 14)
Stash:       f5a5     (depth 14)
Komodo:      f5a5     (depth 14)
Laser:       f5a5     (depth 14)

Performance:
SeaJay:         1875295 nodes @    2090629 nps
Stash:            61919 nodes @    2135137 nps
Komodo:          129024 nodes @    2048001 nps
Laser:           158577 nodes @    1910566 nps

Principal Variations (PV)
What each engine is considering:

SeaJay: f5f6 c6f6 g6f6 f3e4 f6e6 f4f5 e6e7 g3g4 ...
Stash: f5a5 c6c3 g6f5 c3b3 a5d5 g3g4 h5g4 f3g3 ...
Komodo: f5a5 f3g2 a5f5 g2h3 f5d5 c6c8 d5f5 c8g8 ...
Laser: f5a5 f3g2 a5f5 g2h3 f5f6 c6f6 g6f6 g3g4 ...

Move consensus:
  ✓ Strong consensus: f5a5 (Stash, Komodo, Laser)
  f5f6 (SeaJay)
### After the mistake

Four-Engine Position Analysis

FEN: 8/5p2/2R2rk1/7p/5P1P/5KP1/8/8 w - - 0 83
Side to move: White
Search mode: depth = 14
Temp directory: /tmp/chess_analysis_20250901_124243_98148

Engine Results Summary

Scores (as reported by engines):
SeaJay:         +2.28  White +1.50-3.00  
Stash:          +2.43  White +1.50-3.00  
Komodo:         +5.59  White winning     
Laser:          +1.71  White +1.50-3.00  

Best Moves:
SeaJay:      c6f6     (depth 14)
Stash:       c6f6     (depth 14)
Komodo:      c6f6     (depth 14)
Laser:       c6f6     (depth 14)

Performance:
SeaJay:         1214510 nodes @    2130719 nps
Stash:            37368 nodes @    1868400 nps
Komodo:          106937 nodes @    2324717 nps
Laser:            60699 nodes @    1517475 nps

Principal Variations (PV)
What each engine is considering:

SeaJay: c6f6 g6f6 f3e4 f6e6 f4f5 e6e7 e4e5 f7f6 ...
Stash: c6f6 g6f6 f3e4 f6e6 f4f5 e6e7 e4f4 e7f6 ...
Komodo: c6f6 g6f6 f3e4 f6e6 f4f5 e6e7 e4f4 f7f6 ...
Laser: c6f6 g6f6 f3e4 f6e6 f4f5 e6e7 e4f3 f7f6 ...

Move consensus:
  ✓ Unanimous: c6f6 (all engines)

``` [Example Game 1 Full Move History "?"]
[Site "?"]
[Date "2025.09.01"]
[Round "1"]
[White "SeaJay-base"]
[Black "SeaJay-dev"]
[Result "0-1"]
[FEN "r1bqk2r/ppppbpp1/2n2n1p/4p3/2B1P3/2PP1NP1/PP3P1P/RNBQK2R b KQkq - 0 1"]
[SetUp "1"]
[PlyCount "183"]
[GameDuration "00:00:22"]
[GameEndTime "2025-09-01T16:44:03.610 UTC"]
[GameStartTime "2025-09-01T16:43:40.971 UTC"]
[Termination "illegal move"]
[TimeControl "5.95+0.06"]

1...d6 {-0.23 10/21 189 311740}  2.Nbd2 {+0.17 10/21 201 278528}  2...Na5 {-0.16 9/19 68 111484}  3.Bb5+ {+0.22 10/23 177 278233}  3...c6 {-0.38 11/28 195 319488}  4.Ba4 {+0.36 11/26 103 167416}  4...b5 {-0.34 8/21 30 49590}  5.Bc2 {+0.30 10/27 54 80744}  5...c5 {-0.21 9/31 127 180901}  6.O-O {+0.25 10/22 166 250508}  6...Qc7 {-0.33 10/24 191 217088}  7.Kg2 {+0.26 10/24 160 246168}  7...Kf8 {-0.16 9/25 154 243360}  8.d4 {+0.22 8/20 85 129951}  8...cxd4 {-0.13 10/23 131 215414}  9.cxd4 {+0.13 11/23 132 197179}  9...Kg8 {-0.13 10/23 132 205925}  10.a4 {+0.12 9/22 94 137312}  10...Bd7 {+0.06 9/23 97 140097}  11.Re1 {+0.05 10/22 176 234660}  11...Nc6 {-0.05 9/21 114 165401}  12.axb5 {+0.49 12/23 169 235443}  12...Nxd4 {-0.67 12/25 180 253952}  13.Nxd4 {+0.59 11/22 50 83296}  13...exd4 {-0.67 10/26 84 137250}  14.Nb3 {+0.63 10/21 79 128490}  14...Bxb5 {-0.64 9/23 64 110890}  15.Nxd4 {+0.72 9/22 37 53768}  15...Bd7 {-0.81 10/20 194 233472}  16.Bb3 {+0.50 10/23 196 278528}  16...Qb7 {-0.60 7/19 34 58513}  17.Qd3 {+0.75 8/21 57 81340}  17...Rd8 {-0.87 8/25 107 156139}  18.f3 {+1.18 10/21 184 259538}  18...Rc8 {-1.08 9/22 132 193092}  19.Be3 {+1.25 9/19 61 83670}  19...Bd8 {-1.27 10/22 198 274432}  20.Nf5 {+1.13 10/22 198 249856}  20...Bxf5 {-1.15 10/20 115 161178}  21.exf5 {+1.03 10/21 100 131713}  21...a5 {-1.13 10/22 131 189075}  22.Bd4 {+1.12 9/19 106 144035}  22...h5 {-1.00 9/22 136 174107}  23.h4 {+0.92 10/21 199 229376}  23...Rb8 {-1.11 8/19 81 101062}  24.Ra3 {+1.10 10/22 184 247249}  24...d5 {-1.16 10/22 195 196608}  25.Be5 {+1.25 9/19 54 70894}  25...Bc7 {-1.30 11/24 195 233560}  26.Bxf6 {+1.23 12/21 153 236785}  26...gxf6 {-1.38 11/22 129 198716}  27.Re2 {+1.05 11/23 187 294607}  27...Rd8 {-0.81 11/21 163 274941}  28.Ra1 {+0.85 9/19 95 145632}  28...Rh7 {-0.48 10/23 108 179075}  29.f4 {+0.44 10/21 198 282624}  29...d4+ {-0.50 11/21 192 282624}  30.Kh2 {+0.46 9/21 122 190576}  30...Rg7 {-0.50 10/22 127 211767}  31.Bd1 {+0.66 8/17 45 71465}  31...Rh7 {-0.67 10/21 170 284385}  32.Rc1 {+0.70 10/21 163 259411}  32...Qb8 {-0.67 9/18 66 110986}  33.Rc5 {+0.53 10/19 199 286720}  33...Kh8 {-0.60 10/18 139 229799}  34.Rec2 {+0.70 10/21 192 274432}  34...Bb6 {-0.87 11/20 188 249856}  35.Rb5 {+0.92 13/22 159 279873}  35...Qd6 {-0.93 12/19 155 278065}  36.Bf3 {+0.93 12/22 115 200033}  36...Rb8 {-1.08 11/20 86 143971}  37.Rc6 {+0.93 10/17 50 92461}  37...Qd8 {-1.47 14/23 190 259291}  38.Qb3 {+1.58 10/26 123 191049}  38...a4 {-1.82 13/21 168 300143}  39.Qxa4 {+1.91 12/20 191 317447}  39...Kg7 {-2.00 12/22 185 294912}  40.b4 {+2.02 11/22 152 249979}  40...Qe8 {-2.15 11/23 167 272288}  41.Qa6 {+2.39 13/24 216 298665}  41...Qe3 {-1.91 12/23 183 330866}  42.Kg2 {+2.18 12/29 199 331297}  42...Qd2+ {-2.21 12/28 203 327680}  43.Kh3 {+1.85 11/26 162 257778}  43...Qd3 {-1.97 12/24 127 226975}  44.Bg2 {+1.99 13/26 176 311937}  44...Bc7 {-2.07 14/25 211 368640}  45.Ra5 {+2.29 15/25 197 388040}  45...Qxa6 {-2.31 15/24 166 348544}  46.Raxa6 {+2.50 15/22 141 286169}  46...Bd8 {-2.31 13/21 99 214782}  47.Rd6 {+2.50 14/24 74 164635}  47...Be7 {-2.56 14/19 200 319488}  48.Rxd4 {+2.61 14/23 115 235322}  48...Bxb4 {-2.45 14/22 190 315392}  49.Rd7 {+2.69 13/22 92 185246}  49...Re8 {-2.56 12/21 127 226642}  50.Raa7 {+2.42 12/23 205 274432}  50...Rf8 {-2.55 13/23 177 335872}  51.Rab7 {+2.53 12/22 141 269162}  51...Be1 {-2.56 12/22 119 245957}  52.Bd5 {+2.59 12/21 190 282624}  52...Kg8 {-2.65 12/21 80 162249}  53.Rb3 {+2.63 11/22 136 244010}  53...Bf2 {-2.57 11/19 164 239880}  54.Kg2 {+2.65 11/18 55 114831}  54...Bd4 {-2.63 10/20 56 108163}  55.Rd3 {+2.60 11/20 177 299008}  55...Bc5 {-2.54 12/18 71 151669}  56.Rc7 {+2.61 12/20 140 283171}  56...Bb4 {-2.69 12/20 157 258048}  57.Rb7 {+2.60 12/20 152 314280}  57...Ba5 {-2.55 12/20 146 310567}  58.Ra7 {+2.61 10/18 135 262390}  58...Bb6 {-2.58 12/20 145 234209}  59.Ra6 {+2.61 11/20 123 256325}  59...Rb8 {-2.80 12/20 138 225280}  60.Rb3 {+2.67 10/16 21 47130}  60...Rd8 {-2.48 13/21 132 297885}  61.Rbxb6 {+2.52 12/21 151 307200}  61...Rxd5 {-2.29 12/21 49 116430}  62.Rxf6 {+2.46 11/20 41 105875}  62...Kg7 {-2.56 11/25 130 241664}  63.Rad6 {+2.39 11/20 97 216191}  63...Ra5 {-2.46 11/20 104 181428}  64.Ra6 {+2.39 11/21 75 156365}  64...Rd5 {-2.43 11/20 77 178433}  65.Rad6 {+2.27 12/20 144 282624}  65...Ra5 {-2.46 10/18 43 99834}  66.Ra6 {+2.45 12/22 138 262144}  66...Rxa6 {-2.40 12/20 96 213004}  67.Rxa6 {+2.43 12/20 132 249856}  67...Rh8 {-3.03 11/20 119 184320}  68.Rc6 {+2.42 9/17 46 109607}  68...Ra8 {-2.40 8/15 56 79791}  69.f6+ {+2.26 9/19 94 213817}  69...Kg6 {-2.06 10/20 115 266240}  70.Kf2 {+1.80 8/16 56 80993}  70...Ra5 {-1.91 9/17 64 114975}  71.Ke3 {+1.81 10/19 127 272448}  71...Rf5 {-1.89 10/19 62 147644}  72.Ke4 {+1.97 11/20 122 258048}  72...Rb5 {-2.14 12/20 98 230250}  73.Ra6 {+1.97 10/20 97 225533}  73...Rb4+ {-2.21 10/21 105 243094}  74.Ke5 {+1.91 10/21 101 240230}  74...Rb5+ {-3.36 11/23 107 237568}  75.Ke4 {+1.90 10/19 18 42559}  75...Rf5 {-1.88 11/20 84 206725}  76.Rc6 {+2.17 11/19 81 196069}  76...Rb5 {-2.12 11/19 102 134374}  77.Kf3 {+2.02 10/20 94 221320}  77...Kf5 {-2.00 10/20 77 177192}  78.Kf2 {+1.98 10/20 82 191115}  78...Kg6 {-2.14 10/20 95 217555}  79.Ke3 {+1.98 11/20 110 229376}  79...Ra5 {-1.99 11/18 96 225280}  80.Kf3 {+1.99 10/19 107 193828}  80...Rb5 {-1.99 9/17 35 80605}  81.Kf2 {+1.98 10/18 103 134765}  81...Rf5 {-1.88 10/19 61 143772}  82.Kf3 {+1.98 11/22 71 178740}  82...Rxf6 ?? {-2.15 12/21 92 212237}  83.Rxf6+ {+2.16 11/21 66 156391}  83...Kxf6 {-2.15 11/16 7 17740}  84.Ke4 {+2.51 15/23 106 241494}  84...Ke6 {-2.45 17/26 95 286990}  85.f5+ {+2.45 16/23 61 184339}  85...Ke7 {-2.59 16/28 44 141740}  86.g4 {+2.88 17/25 96 278528}  86...hxg4 {-3.25 19/28 96 312981}  87.h5 {+3.25 18/27 84 263816}  87...f6 {-2.96 18/32 93 171623}  88.Kf4 {+3.20 22/28 97 288569}  88...Kf8 {-2.96 18/32 58 121844}  89.h6 {+3.25 22/31 36 152334}  89...Kg8 {-3.25 21/31 74 290166}  90.Kxg4 {+2.97 25/29 241 1224704}  90...Kh8 {-2.96 20/23 19 102794}  91.Kh4 {+3.20 25/28 327 2105344}  91...Kg8 {-3.25 21/23 18 97225}  92.Kg4 {+3.25 25/29 320 2068480}  92...Kh8 {-3.25 24/25 108 421888, White makes an illegal move: h4g4}  0-1


```
## Example Game 2

### Before the mistake

FEN: r1b1k2r/pp4pp/3Bpp2/3p4/4q3/8/PQ3PPP/1R2R1K1 b kq - 1 16
Side to move: Black
Search mode: depth = 14
Temp directory: /tmp/chess_analysis_20250901_125750_99925

Engine Results Summary

Scores (as reported by engines):
SeaJay:         +2.38  Black +1.50-3.00  
Stash:          +0.11  Black +0.00-0.20  
Komodo:         -0.17  White -0.00-0.20  
Laser:          -0.33  White -0.20-0.50  

Best Moves:
SeaJay:      e4d3     (depth 14)
Stash:       e4f5     (depth 14)
Komodo:      e4d3     (depth 14)
Laser:       e4f5     (depth 14)

Performance:
SeaJay:         1466997 nodes @     708352 nps
Stash:           185339 nodes @     565057 nps
Komodo:          210660 nodes @     718975 nps
Laser:           246248 nodes @     571341 nps

==========================================
Principal Variations (PV)

What each engine is considering:

SeaJay: e4d3 b1d1 d3c4 d1c1 c4a6 b2b4 c8d7 a2a4 ...
Stash: e4f5 b1c1 e6e5 b2b5 c8d7 b5b7 e8f7 c1c7 ...
Komodo: e4d3 e1c1 d3a6
Laser: e4f5 b1c1 b7b6 c1c7 c8d7 f2f4 a8d8 c7a7 ...

Move consensus:
  Split: e4d3 (SeaJay, Komodo)
  Split: e4f5 (Stash, Laser)

### After the mistake

FEN: r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/1R2R1K1 w kq - 2 17
Side to move: White
Search mode: depth = 14
Temp directory: /tmp/chess_analysis_20250901_125921_975

Engine Results Summary

Scores (as reported by engines):
SeaJay:         -1.49  Black -0.50-1.50  
Stash:          +0.80  White +0.50-1.50  
Komodo:         +1.73  White +1.50-3.00  
Laser:          +0.76  White +0.50-1.50  

Best Moves:
SeaJay:      b1c1     (depth 14)
Stash:       h2h3     (depth 14)
Komodo:      h2h3     (depth 14)
Laser:       h2h3     (depth 14)

Performance:
SeaJay:         3757583 nodes @     840434 nps
Stash:           127342 nodes @     767120 nps
Komodo:          222523 nodes @     680498 nps
Laser:           432588 nodes @     660439 nps

Principal Variations (PV)

What each engine is considering:

SeaJay: b1c1 e8f7 c1c7 f7g6 b2b1 g4f5 b1f5 g6f5 ...
Stash: h2h3 g4f5 b1c1 c8d7 b2b7 a8d8 c1c7 e6e5 ...
Komodo: h2h3 g4g6 b1c1
Laser: h2h3 g4f5 b1c1 e6e5 c1c7 f5e6 d6e5 d5d4 ...

Move consensus:
  ✓ Strong consensus: h2h3 (Stash, Komodo, Laser)
  b1c1 (SeaJay)


``` [Example Game 2 Full Moves"?"]
[Site "?"]
[Date "2025.08.27"]
[Round "?"]
[White "Brandon Harris"]
[Black "SeaJay 20250827"]
[Result "1-0"]
[ECO "A40"]
[PlyCount "68"]
[TimeControl "600:300"]

1.d4 e6 2.Nf3 c5 3.c3 cxd4 4.cxd4 Nf6 5.Bf4 Nc6 6.Nc3 Qb6 7.Qc2 Nxd4 8.Nxd4 Qxd4 9.e4 Bb4 10.Bd3 Nxe4 11.Bxe4 d5 12.O-O Bxc3 13.Bd6 Bxb2 ?? 14.Rab1 Qxe4 15.Qxb2 f6 16.Rfe1 Qg4 ?? 17.Rbc1 Kf7 ?? 18.Rc7+ Kg6 19.h3 Qf5 ?? 20.Re3 e5 21.Rg3+ Kh5 ?? 22.Qe2+ Kh6 23.Rgxg7 Bd7 24.Rcxd7 Qf4 25.Qc2 f5 26.Bb8 Qe4 27.Rd6+ Kh5 28.Re6 Qe1+ 29.Kh2 Qxf2 30.Qd1+ Qe2 31.Rf7 Kh4 32.Rh6+ Kg5 33.Rxf5+ Kxf5 34.Qxe2 Kg5 1-0

