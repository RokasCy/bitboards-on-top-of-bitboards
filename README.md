# bitboards-on-top-of-bitboards
This is a playable chess engine coded from scratch inspired by Sebastian Lagues video on devoping a chess bot 

## Overview

The C++ engine is compiled into a python module using pybind11 and is accessed by ChessGUI.py which renders the chess board with the help of pygame

I chose to wrap the engine with python since working in c++ libraries was too much of a pain, but i ended up having to use pybind11 (a c++ lib) to connect the scripts anyways...

## Board Representation

To represent the board position we employ the help of bitboards. A bitboard is a 64bit binary number that stores the positions of each piece (1 means the square is occupied and 0 means its empty). Due to the fact that a chess board contains 64 squares we can describe the board position using 12 of these bitboards (1 for each unique piece type). Effectively describing the board position using 12 large numbers

This turns out to be a very efficient way of representing the board since we can perform various of bitwise operations to move pieces around with minimal computational overhead

## Engine Search

The Engine employs an algorithm called minimax to calculate the best posible move. 

- The engine first generates all legal moves and plays each one of them
- After playing a move it then generates all legal responses from the other side and plays them.
- After some amount of cycles of this (dependint on the search depth specified) it evalueates the board position. 
- For the evaluation of intermediate nodes (moves) it calculates the best move for the side

## Instructions

To install python requirements run in a terminal
```bash
pip install -r requirements.txt
```
CMake is required. Pybind11 is inside the repo so no need to do anything

To build the program create a build folder and run these commands from the directory
```bash
cmake ..
cmake --build .
```
This will create a .pyd inside build/Debug/
to finally run the program make sure correct directory to the .pyd is linked inside ChessGUI.py and run the script.





#

P.S. No en passant