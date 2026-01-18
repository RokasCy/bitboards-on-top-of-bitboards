#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "engine.h"
#include "move_generator.h"

namespace py = pybind11;

PYBIND11_MODULE(chess_engine, m) {

    py::class_<Move>(m, "Move")
        .def(py::init<uint8_t, uint8_t, uint8_t>())
        .def_readwrite("from_", &Move::from)
        .def_readwrite("to_", &Move::to)
        .def_readwrite("flags", &Move::flags);
    
    
    py::class_<Board>(m, "Board")
        .def(py::init<>())
        .def_readonly("squares", &Board::squares)
        .def_readonly("CHECKMATED", &Board::CHECKMATED)
        .def("set_up_board", &Board::set_up_board)
        .def("player_move", &Board::player_move)
        .def("make_move", &Board::make_move)
        .def("undo_move", &Board::undo_move)
        .def("generate_piece_moves", &Board::generate_piece_moves)
        .def("generate_all_moves", &Board::generate_all_moves)
        .def("generate_legal_moves", &Board::generate_legal_moves)
        .def("evaluation", &Board::evaluation)
        .def("get_minimax_move", &Board::get_minimax_move)
        .def("minimax_search", &Board::minimax_search)
        .def("get_flags", &Board::get_flags)

        .def("update_attack_info", &Board::update_attack_info);


}