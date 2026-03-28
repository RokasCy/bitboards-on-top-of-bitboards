import pygame
from enum import Enum
import os


project_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
image_dir = os.path.join(project_dir, "assets", "pieces-png")

class Piece(Enum): 
    WHITE_PAWN = 0
    WHITE_KNIGHT = 1
    WHITE_BISHOP = 2
    WHITE_ROOK = 3
    WHITE_QUEEN = 4
    WHITE_KING = 5
    BLACK_PAWN = 6
    BLACK_KNIGHT = 7
    BLACK_BISHOP = 8
    BLACK_ROOK = 9
    BLACK_QUEEN = 10
    BLACK_KING = 11
    EMPTY = 12

class Side(Enum):
    WHITE = 0
    BLACK = 1

class Drag:
    def __init__(self):
        self.piece = Piece.EMPTY
        self.start = -1
        self.pixel_pos = (0, 0)
    

#piece_directions = {Piece.WHITE_QUEEN}  


def safe_image_load(filename, size):
    path = os.path.join(image_dir , f"{filename}.png")
    try: 
        img = pygame.image.load(path).convert_alpha()
        
    except (FileNotFoundError, pygame.error) as er:
        print(f"Failed to load {filename}: {er}")
        fallback_path = os.path.join(image_dir, "error-icon.png")
        img = pygame.image.load(fallback_path).convert_alpha()
    
    return pygame.transform.smoothscale(img, (size, size))


def pieces_image_setup(size, player_side):
    piece_image = {}

    for p in Piece:
        if p == Piece.EMPTY: continue

        if(player_side == 1):
            img = safe_image_load(p.name, size)
            piece_image[p] = img
        elif (player_side == 2):
            color, type = p.name.split("_")
            pname = ""
            if color == "WHITE":
                pname = "BLACK_" + type
            else:
                pname = "WHITE_" + type
            
            img = safe_image_load(pname, size)
            piece_image[p] = img

    return piece_image

