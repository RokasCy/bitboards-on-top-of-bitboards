import sys
sys.path.append("E:/Git/bitboards-on-top-of-bitboards/build/Debug")
import chess_engine

import pygame
from pieces import Piece, pieces_image_setup, Drag, Side


pygame.init()


def coord_to_index(x, y):
    return x + y*8

def get_side(piece):
    if piece <= 5:
        return Side.WHITE
    return Side.BLACK

class Renderer:

    board_colour = [(213, 194, 183), (145, 85, 51)]
    move_colour = (25, 25, 25, 150)
    once = False

    def __init__(self):

        self.square_size = BOARD_SIZE // 8
        self.piece_image = pieces_image_setup(self.square_size)

        self.board_layer = pygame.Surface((BOARD_SIZE, BOARD_SIZE))
        self.piece_layer = pygame.Surface((BOARD_SIZE, BOARD_SIZE), pygame.SRCALPHA)
        self.highlight_layer = pygame.Surface((BOARD_SIZE, BOARD_SIZE), pygame.SRCALPHA)
        self.checkmate_layer = pygame.Surface((BOARD_SIZE, BOARD_SIZE), pygame.SRCALPHA)


    def draw_board(self):
        size = self.square_size
        for y in range(8):
            for x in range(8):
                colour = Renderer.board_colour[(x+y) % 2]
                pygame.draw.rect(self.board_layer, 
                                colour, 
                                (x*size, y*size, size, size))
        
        window.blit(self.board_layer)


    def draw_pieces(self, drag):
        self.piece_layer.fill((0, 0, 0, 0))

        size = self.square_size
        for y in range(8):
            for x in range(8):
                
                index = coord_to_index(x, 7-y)

                #current piece being dragged
                if(index == drag.start): continue

                piece = board.squares[index]
                
                #convert to enum var
                piece = Piece(piece)

                if piece != Piece.EMPTY:
                    self.piece_layer.blit(self.piece_image[piece], 
                                          (x*size, y*size))

        offset = self.square_size // 2


        if drag.piece != Piece.EMPTY:
            self.piece_layer.blit(self.piece_image[drag.piece], 
                                  (drag.pixel_pos[0] - offset, drag.pixel_pos[1] - offset))
        
        window.blit(self.piece_layer)
    
    def draw_highlights(self, legal_moves):
        self.highlight_layer.fill((0, 0, 0, 0))

        if legal_moves is None: return 

        offset = self.square_size // 2

        for square in legal_moves:
            rank = 7-square.to_ // 8
            file = square.to_ % 8

            if (square.flags == 1):
                pygame.draw.circle(self.highlight_layer, renderer.move_colour,
                               (file*self.square_size+offset, rank*self.square_size+offset),
                               radius=offset, width=offset//6)
            else:
                pygame.draw.circle(self.highlight_layer, renderer.move_colour,
                               (file*self.square_size+offset, rank*self.square_size+offset),
                               radius=offset//4)
            

        window.blit(self.highlight_layer)
    
    def checkmate(self, CHECKMATED):
        if CHECKMATED[0]:
            pygame.draw.rect(self.checkmate_layer, (150, 150, 150, 75), (0, 0, BOARD_SIZE, BOARD_SIZE))
            if not Renderer.once: 
                print("WHITE IS CHECKMATED")
                Renderer.once = True
        elif CHECKMATED[1]:
            pygame.draw.rect(self.checkmate_layer, (150, 150, 150, 75), (0, 0, BOARD_SIZE, BOARD_SIZE))
            if not Renderer.once: 
                print("BLACK IS CHECKMATED")
                Renderer.once = True

        window.blit(self.checkmate_layer)
            


def index_to_cord(index):
    return (index % 8, index // 8)


class Controller:
    def __init__(self):

        self.drag = Drag()
        self.move = chess_engine.Move(0, 0, 0)

        self.turn = Side.WHITE
        self.legal_moves = None

        self.robot = False
        self.player_side = Side.WHITE


    def pickup(self, event):
        x = (event.pos[0]*8//BOARD_SIZE)
        y = 7-(event.pos[1]*8//BOARD_SIZE)

        from_index = coord_to_index(x, y)
        piece = board.squares[from_index]

        if Piece(piece) == Piece.EMPTY: return

        #when playing against robot
        if self.robot and get_side(piece) != self.player_side: return
        if get_side(piece) != self.turn : return


        self.drag.piece = Piece(piece)
        self.drag.start = from_index
        self.move.from_ = from_index

        self.legal_moves = board.generate_legal_moves(board.generate_piece_moves(piece, from_index), self.turn.value, False)
    
    def putdown(self):
        x = self.drag.pixel_pos[0] * 8 // BOARD_SIZE
        y = 7-self.drag.pixel_pos[1] * 8 // BOARD_SIZE

        to_index = coord_to_index(x, y)

        legal = False

        for move in self.legal_moves:
            if to_index == move.to_ :
                legal = True
                break
        
        #if not legal and self.drag.start != to_index : return

        self.move.to_ = to_index
        board.get_flags(self.move, self.turn.value)

        #turn handling
        if(self.drag.start != to_index):
            board.player_move(self.move)

            self.turn = Side.BLACK if self.turn == Side.WHITE else Side.WHITE
        
            if(self.robot):
                robot_move, eval = board.get_minimax_move(self.turn.value)
                board.player_move(robot_move)
                #print(robot_move.from_, robot_move.to_, robot_move.flags)
                print(f"evaluation:{eval}\n")

                self.turn = Side.BLACK if self.turn == Side.WHITE else Side.WHITE

        self.drag.piece = Piece.EMPTY
        self.drag.start = -1
        self.legal_moves = None
        self.move.flags = 0

        #check if checkmated
        board.generate_legal_moves(board.generate_all_moves(self.turn.value), self.turn.value, True)
        


BOARD_SIZE = 800
window = pygame.display.set_mode((BOARD_SIZE, BOARD_SIZE))
pygame.display.set_caption("Chess Engine")

board = chess_engine.Board()
board.set_up_board()

renderer = Renderer()
controller = Controller()

running = True
while running:
    for event in pygame.event.get():

        if event.type == pygame.QUIT:
            running = False

        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            if controller.drag.piece == Piece.EMPTY:
                controller.pickup(event)
            else:
                controller.putdown()
        

        if controller.drag.piece != Piece.EMPTY:
            controller.drag.pixel_pos = pygame.mouse.get_pos()

    renderer.draw_board()
    renderer.draw_highlights(controller.legal_moves)
    renderer.draw_pieces(controller.drag)
    renderer.checkmate(board.CHECKMATED)
    
    pygame.display.flip()
        
