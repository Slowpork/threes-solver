#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <random>
#include <vector>
#include <unordered_map>

#include <cctype>
#include <cassert>
#include <cstdio>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

enum class PlayerMove {
  UNKNOWN,
  SWIPE_UP,
  SWIPE_DOWN,
  SWIPE_LEFT,
  SWIPE_RIGHT,
};

template <class T>
bool
is_power_of_two(T a) {
  return a && !(a & (a - 1));
}

template <class T>
T
log_base_2(T a) {
  T toret = 0;
  while (a >>= 1) {
    toret += 1;
  }
  return toret;
}

class Card {
  typedef unsigned card_value_type;

  card_value_type _value;

public:
  static
  bool
  is_valid_3_card_value(card_value_type a) {
    return !(a % 3) && is_power_of_two(a / 3);
  }

  static
  bool
  is_valid_card_value(card_value_type a) {
    return (a == 1 || a == 2 || is_valid_3_card_value(a));
  }

  Card(card_value_type value) : _value(value) {
    if (!is_valid_card_value(value)) throw std::runtime_error("bad card value");
  }

  Card() : _value(0) {}

  card_value_type
  value() const {
    return _value;
  }

  Card
  combine(Card b) const {
    if (value() == 1 && b.value() == 1) throw std::runtime_error("can't combine cards");
    return Card(value() + b.value());
  }

  bool
  operator==(const Card &b) const {
    return value() == b.value();
  }

  bool
  operator!=(const Card &b) const {
    return !(*this == b);
  }
};

static
bool
can_combine(const Card & a, const Card & b) {
  return (!(a.value() == 1 && b.value() == 1)) && Card::is_valid_card_value(a.value() + b.value());
}

const Card nullcard;

struct CardVector {
  int dx;
  int dy;
};

struct CardPosition {
  size_t x;
  size_t y;

  CardPosition &
  operator+=(const CardVector & v) {
    x += v.dx;
    y += v.dy;
    return *this;
  }

  CardPosition &
  operator-=(const CardVector & v) {
    x -= v.dx;
    y -= v.dy;
    return *this;
  }

  CardPosition
  operator+(const CardVector & v) {
    CardPosition new_card_pos = *this;
    new_card_pos.x += v.dx;
    new_card_pos.y += v.dy;
    return new_card_pos;
  }
};

struct CardPlacement {
  Card card;
  CardPosition position;
};

enum class NextColor {
  RED, BLUE, WHITE
};

std::string
lower_string(std::string data) {
  std::transform(data.begin(), data.end(), data.begin(), [] (const char & a) {
      return std::tolower(a);});
  return data;
}

std::string
strip_string(std::string data) {
  auto beginning_it = data.begin();
  auto ending_it = data.end();

  while (std::isspace(*beginning_it)) {
    beginning_it++;
  }

  while (std::isspace(*(ending_it - 1))) {
    ending_it--;
  }

  return std::string(beginning_it, ending_it);
}

NextColor
human_string_to_next_color(std::string str) {
  auto lowered_string = strip_string(lower_string(str));
  if (str == "blue") return NextColor::BLUE;
  if (str == "red") return NextColor::RED;
  if (str == "white") return NextColor::WHITE;
  throw std::runtime_error("invalid color");
}

std::string
next_color_to_human_string(NextColor nc) {
  switch (nc) {
  case NextColor::BLUE: return "blue";
  case NextColor::RED: return "red";
  case NextColor::WHITE: return "white";
  default: assert(false);
  }
}

class Board {
public:
  const static size_t BOARD_SIZE = 4;
  const static size_t BOARD_ELTS = BOARD_SIZE * BOARD_SIZE;

private:
  Card board[BOARD_ELTS];
  NextColor nc;

public:
  static
  bool
  is_valid_card_position(const CardPosition & pos) {
    return pos.x < BOARD_SIZE && pos.y < BOARD_SIZE;
  }

private:
  Card &
  operator[](const CardPosition & pos) {
    assert(is_valid_card_position(pos));
    return board[pos.x + pos.y * BOARD_SIZE];
  }

  void
  insert(const CardPlacement & cp) {
    (*this)[cp.position] = cp.card;
  }

  void
  erase(const CardPosition & pos) {
    (*this)[pos] = nullcard;
  }

  bool
  _shift_inner(const PlayerMove & pm, bool mutate) {
    // the beam_start_pos is the starting point of the "shift beam"
    CardPosition beam_start_pos;
    // the 'beam vector' moves us to each card we should shift
    // starting from the beam_pos
    CardVector beam_vector;
    // the 'shift vector' points to the direction a card should be shifted
    CardVector shift_vector;

    switch (pm) {
    case PlayerMove::SWIPE_UP: {
      beam_start_pos = {0, 1};
      beam_vector = {1, 0};
      shift_vector = {0, -1};
      break;
    };
    case PlayerMove::SWIPE_DOWN: {
      beam_start_pos = {0, 2};
      beam_vector = {1, 0};
      shift_vector = {0, 1};
      break;
    }
    case PlayerMove::SWIPE_LEFT: {
      beam_start_pos = {1, 0};
      beam_vector = {0, 1};
      shift_vector = {-1, 0};
      break;
    }
    case PlayerMove::SWIPE_RIGHT: {
      beam_start_pos = {2, 0};
      beam_vector = {0, 1};
      shift_vector = {1, 0};
      break;
    }
    default: assert(false);
    }

    bool changed = false;
    for (size_t i = 0; i < Board::BOARD_SIZE - 1; ++i) {
      auto card_to_shift_pos = beam_start_pos;
      for (size_t j = 0; j < Board::BOARD_SIZE; ++j) {
        auto card_to_shift_onto_pos = card_to_shift_pos + shift_vector;

        if ((*this)[card_to_shift_pos] != nullcard &&
            can_combine((*this)[card_to_shift_pos],
                        (*this)[card_to_shift_onto_pos])) {
          if (!mutate) return true;
          (*this)[card_to_shift_onto_pos] = (*this)[card_to_shift_pos].combine((*this)[card_to_shift_onto_pos]);
          erase(card_to_shift_pos);
          if (!changed) changed = true;
        }

        card_to_shift_pos += beam_vector;
      }
      beam_start_pos -= shift_vector;
    }

    return changed;
  }

public:
  template <class Range>
  Board(const Range & r, NextColor nc_) : nc(nc_) {
    for (const auto & cp : r) {
      if (!is_valid_card_position(cp.position)) throw std::runtime_error("bad card position");
      if ((*this)[cp.position] != nullcard) throw std::runtime_error("same card twice!");
      (*this)[cp.position] = cp.card;
    }
  }

  NextColor
  next_color() const {
    return nc;
  }

  const Card &
  operator[](const CardPosition & pos) const {
    assert(is_valid_card_position(pos));
    return board[pos.x + pos.y * BOARD_SIZE];
  }

  bool
  can_shift(const PlayerMove & pm) const {
    return const_cast<Board &>(*this)._shift_inner(pm, false);
  }

  void
  shift(const PlayerMove & pm) {
    auto shifted = _shift_inner(pm, true);
    if (!shifted) throw std::runtime_error("can't shift");
  }

  void
  computers_move(const PlayerMove & pm, const CardPlacement & cp, NextColor nc_) {
    bool valid_for_swipe;
    switch (pm) {
    case PlayerMove::SWIPE_UP: {
      valid_for_swipe = cp.position.y == BOARD_SIZE - 1;
      break;
    }
    case PlayerMove::SWIPE_DOWN: {
      valid_for_swipe = cp.position.y == 0;
      break;
    }
    case PlayerMove::SWIPE_LEFT: {
      valid_for_swipe = cp.position.x == BOARD_SIZE - 1;
      break;
    }
    case PlayerMove::SWIPE_RIGHT: {
      valid_for_swipe = cp.position.x == 0;
      break;
    }
    default: assert(false); valid_for_swipe = false;
    }

    if (!valid_for_swipe) throw std::runtime_error("can't place card there");

    if ((*this)[cp.position] != nullcard) throw std::runtime_error("can't place card there");
    (*this)[cp.position] = cp.card;
    nc = nc_;
  }

  Card
  max_card() const {
    return *std::max_element(board, board + BOARD_ELTS,
                             [] (const Card & a, const Card & b) {
                               return a.value() < b.value();
                             });
  }

  static
  int
  _int_print_width(int base, int a) {
    if (a == 0) return 1;
    int toret = a < 0 ? 1 : 0;
    while (a) {
      ++toret;
      a /= base;
    }
    return toret;
  }

  void
  print_board(std::ostream & out) const {
    /* first find max width */
    auto max_width = _int_print_width(10, max_card().value());

    for (size_t j = 0; j < BOARD_SIZE; ++j) {
      for (size_t i = 0; i < BOARD_SIZE; ++i) {
        out << std::setw(max_width) << std::dec << std::right;
        out << (*this)[{i, j}].value() << ' ';
      }
      out << std::endl;
    }
  }

  bool
  is_empty() const {
    for (const auto & card : board) {
      if (card != nullcard) return false;
    }
    return true;
  }
};

// some printers for our custom types

template <class Traits>
std::basic_ostream<char,Traits> &
operator<<(std::basic_ostream<char,Traits> & os,
           const CardPosition & pos) {
  os << "CardPosition(" << pos.x << ", " << pos.y << ")";
  return os;
}

template <class Traits>
std::basic_ostream<char,Traits> &
operator<<(std::basic_ostream<char,Traits> & os,
           const Card & card) {
  os << "Card(" << card.value() << ")";
  return os;
}

std::string
to_string(const PlayerMove & pm) {
  switch (pm) {
  case PlayerMove::SWIPE_UP: return "SWIPE_UP";
  case PlayerMove::SWIPE_DOWN: return "SWIPE_DOWN";
  case PlayerMove::SWIPE_LEFT: return "SWIPE_LEFT";
  case PlayerMove::SWIPE_RIGHT: return "SWIPE_RIGHT";
  case PlayerMove::UNKNOWN: return "UNKNOWN";
  }
  /* notreached */
  assert(false);
  return "";
}

template <class Traits>
std::basic_ostream<char,Traits> &
operator<<(std::basic_ostream<char,Traits> & os,
           const PlayerMove & pm) {
  return os << to_string(pm);
}

typedef double board_score_t;

static
board_score_t
card_friction(const Card & a_, const Card & b_) {
  auto a = a_.value();
  auto b = b_.value();

  // friction is commutative, so order args
  if (b < a) {
    auto tmp = b;
    b = a;
    a = tmp;
  }

  if (a == 0) return 0;

  int add_to_power_of_two_friction = 0;
  if (a == 1) {
    switch (b) {
    case 0: assert(false);
    case 1: return 2;
    case 2: return 0;
    default: {
      a = 3;
      add_to_power_of_two_friction = 1;
      break;
    }
    }
  }
  else if (a == 2) {
    switch (b) {
    case 0: case 1: assert(false);
    case 2: return 2;
    default: {
      a = 3;
      add_to_power_of_two_friction = 1;
      break;
    }
    }
  }

  assert(Card::is_valid_3_card_value(a));
  assert(Card::is_valid_3_card_value(b));

  return add_to_power_of_two_friction + log_base_2(b / 3) - log_base_2(a / 3);
}

static
board_score_t
compute_board_friction(const Board & board) {
  board_score_t board_friction = 0;

  // first sum up the horizontal frictions
  for (unsigned x = 0; x < Board::BOARD_SIZE - 1; ++x) {
    for (unsigned y = 0; y < Board::BOARD_SIZE; ++y) {
      board_friction += card_friction(board[{x, y}], board[{x + 1, y}]);
    }
  }

  // then the vertical friction
  for (unsigned x = 0; x < Board::BOARD_SIZE; ++x) {
    for (unsigned y = 0; y < Board::BOARD_SIZE - 1; ++y) {
      board_friction += card_friction(board[{x, y}], board[{x, y + 1}]);
    }
  }

  return board_friction;
}

static
board_score_t
board_evaluator(const Board & board) {
  return board.max_card().value() / compute_board_friction(board);
}

static
bool
game_is_over(const Board & board) {
  for (auto move : {
         PlayerMove::SWIPE_UP,
         PlayerMove::SWIPE_DOWN,
         PlayerMove::SWIPE_LEFT,
         PlayerMove::SWIPE_RIGHT}) {
    if (board.can_shift(move)) return false;
  }

  return true;
}

static
std::vector<CardPlacement>
possible_computer_card_placements_post_shift(const Board & board, PlayerMove pm) {
  std::vector<CardPlacement> toret;

  CardPosition pos;
  CardVector vector;

  switch (pm) {
  case PlayerMove::SWIPE_UP: {
    pos = {0, 3};
    vector = {1, 0};
    break;
  }
  case PlayerMove::SWIPE_DOWN: {
    pos = {0, 0};
    vector = {1, 0};
    break;
  }
  case PlayerMove::SWIPE_LEFT: {
    pos = {3, 0};
    vector = {0, 1};
    break;
  }
  case PlayerMove::SWIPE_RIGHT: {
    pos = {0, 0};
    vector = {0, 1};
    break;
  }
  default: assert(false);
  }

  for (size_t i = 0; i < Board::BOARD_SIZE; ++i, pos += vector) {
    if (board[pos] != nullcard) continue;

    switch (board.next_color()) {
    case NextColor::BLUE: toret.push_back({1, pos}); break;
    case NextColor::RED: toret.push_back({2, pos}); break;
    case NextColor::WHITE: {
      toret.push_back({3, pos});
      for (Card card = 6; card.value() < board.max_card().value(); card = card.combine(card)) {
        toret.push_back({card, pos});
      }
      break;
    }
    default: assert(false);
    }
  }

  return toret;
}

struct MinimaxResult {
  PlayerMove best_move;
  board_score_t move_score;
  bool death_guaranteed;
};

template <class F>
MinimaxResult
inner_alphabeta(F evaluator,
                const Board & board,
                unsigned depth,
                board_score_t alpha, board_score_t beta) {
  if (!depth) {
    auto is_game_over = game_is_over(board);
    return {PlayerMove::UNKNOWN, is_game_over ? std::numeric_limits<board_score_t>::lowest() : evaluator(board), is_game_over};
  }

  // iterate over player's move
  bool death_guaranteed = true;
  PlayerMove best_move = PlayerMove::UNKNOWN;
  for (auto move : {
         PlayerMove::SWIPE_UP,
         PlayerMove::SWIPE_DOWN,
         PlayerMove::SWIPE_LEFT,
         PlayerMove::SWIPE_RIGHT}) {
    if (!board.can_shift(move)) continue;

    auto board2 = board;
    board2.shift(move);

    // iterate over computer's move
    auto computer_moved = false;
    auto new_beta = beta;
    for (const auto & cp : possible_computer_card_placements_post_shift(board2, move)) {
      computer_moved = true;

      for (const auto & nc2 : {NextColor::RED, NextColor::BLUE, NextColor::WHITE}) {
        // copy board & modify it
        auto board3 = board2;
        board3.computers_move(move, cp, nc2);

        auto res = inner_alphabeta(evaluator, board3, depth - 1,
                                   alpha, new_beta);
        if (!res.death_guaranteed) death_guaranteed = false;
        if (res.move_score < new_beta) {
          new_beta = res.move_score;
        }

        if (new_beta <= alpha) break;
      }
    }

    // the computer has no moves if and only if the player has no moves
    assert(computer_moved);

    if (best_move == PlayerMove::UNKNOWN ||
        new_beta > alpha) {
      best_move = move;
      alpha = new_beta;
    }

    if (beta <= alpha) break;
  }

  if (best_move == PlayerMove::UNKNOWN) {
    // no move was available
    return {PlayerMove::UNKNOWN, std::numeric_limits<board_score_t>::lowest(), true};
  }

  return {best_move, alpha, death_guaranteed};
}

template <class F>
PlayerMove
run_minimax(F evaluator, const Board & board) {
  // we run a basic alpha-beta minimax
  // where the computer places the next card is considered it's move
  auto ret = inner_alphabeta(evaluator, board, 4,
                             std::numeric_limits<board_score_t>::lowest(),
                             std::numeric_limits<board_score_t>::max());
  if (ret.death_guaranteed) {
    std::cout << "Death is unavoidable at this point" << std::endl;
  }

  return ret.best_move;
}

Board
read_board_from_human_input(std::istream & is) {
  // first read next color
  std::string next_color_str;
  is >> next_color_str;
  if (!is) throw std::runtime_error("input valid");
  auto next_color = human_string_to_next_color(next_color_str);

  std::vector<CardPlacement> board_init;
  for (unsigned j = 0; j < Board::BOARD_SIZE; ++j) {
    for (unsigned i = 0; i < Board::BOARD_SIZE; ++i) {
      unsigned card_value;
      is >> card_value;
      if (!is) throw std::runtime_error("invalid card!");
      if (!card_value) continue;
      Card c = card_value;
      board_init.push_back({c, {i, j}});
    }
  }

  return { board_init, next_color };
}

#ifdef EMSCRIPTEN

extern "C" {

void
web_worker(char *data, size_t size) {
  auto is = std::istringstream(std::string(data, data + size));
  auto board = read_board_from_human_input(is);
  auto player_move = run_minimax(board_evaluator, board);
  char response_mut[sizeof(player_move)];
  memcpy(response_mut, &player_move, sizeof(player_move));
  emscripten_worker_respond(response_mut, sizeof(player_move));
}

void *
create_board(const char *textual_representation) {
  try {
    auto is = std::istringstream(textual_representation);
    auto board = read_board_from_human_input(is);
    return (void *) new Board(board);
  }
  catch (...) {
    return nullptr;
  }
}

size_t
serialize_board(void *board, char *str, size_t size) {
  auto board_p = (const Board *) board;

  std::ostringstream serialized;

  serialized << next_color_to_human_string(board_p->next_color()) << " ";

  for (size_t y = 0; y < Board::BOARD_SIZE; ++y) {
    for (size_t x = 0; x < Board::BOARD_SIZE; ++x) {
      serialized << (*board_p)[{x, y}].value() << " ";
    }
  }

  auto ret_string = serialized.str();
  auto to_copy = std::min(size, ret_string.size());
  std::memmove(str, ret_string.data(), to_copy);

  return to_copy;
}

void
free_board(void *b) {
  delete [] (char *) b;
}

worker_handle
create_worker(const char *url) {
  return emscripten_create_worker(url);
}

typedef void (*my_cb_t)(const char *);

void callback_js(char *data, int size, void *a) {
  PlayerMove pm;
  memcpy(&pm, data, size);
  auto str_pm = to_string(pm);
  auto cb = (my_cb_t) a;
  cb(str_pm.c_str());
}

void
get_next_move(worker_handle wh, void *board, my_cb_t cb) {
  char serialized_board[256];
  auto actual_size = serialize_board(board, serialized_board, sizeof(serialized_board));
  emscripten_call_worker(wh, "web_worker",
                         serialized_board, actual_size,
                         callback_js, (void *) cb);
}

PlayerMove
player_move_from_string(std::string a) {
  PlayerMove pm;
  if (a == "SWIPE_UP") pm = PlayerMove::SWIPE_UP;
  else if (a == "SWIPE_DOWN") pm = PlayerMove::SWIPE_DOWN;
  else if (a == "SWIPE_LEFT") pm = PlayerMove::SWIPE_LEFT;
  else if (a == "SWIPE_RIGHT") pm = PlayerMove::SWIPE_RIGHT;
  else throw std::runtime_error("bad players move");
  return pm;
}

int
make_computers_move(void *board,
                    const char *players_move,
                    unsigned card_value, size_t card_position_x, size_t card_position_y,
                    const char *human_readable_next_color) {
  auto board_p = (Board *) board;
  try {
    auto pm = player_move_from_string(players_move);
    auto next_color = human_string_to_next_color(human_readable_next_color);
    board_p->computers_move(pm, {card_value, {card_position_x, card_position_y}},
                            next_color);
    return true;
  }
  catch (...) {
    return false;
  }
}

int
shift_board(void *board,
            const char *players_move) {
  auto board_p = (Board *) board;
  try {
    auto pm = player_move_from_string(players_move);
    board_p->shift(pm);
    return true;
  }
  catch (...) {
    return false;
  }
}


}

#else

class InvalidCardPlacementLine : public std::exception {};
class FinishedEnteringCardPlacement : public std::exception {};
class EOFError : public std::exception {};

static
CardPlacement
get_card_placement_line(decltype(std::cin) & cin_, bool allow_done) {
  std::string line;
  getline(cin_, line);

  if (cin_.eof()) throw EOFError();

  if (allow_done && line == "done") throw FinishedEnteringCardPlacement();

  std::istringstream is(line);
  unsigned card_value;
  CardPosition pos;
  is >> card_value >> pos.x >> pos.y;

  if (is.fail() || !Board::is_valid_card_position(pos)) throw InvalidCardPlacementLine();

  return { card_value, pos };
}

struct ComputersResponse {
  CardPlacement card_placement;
  NextColor next_color;
};

class PrintCurrentBoard {
public:
  void
  current_board(const Board & board) {
    board.print_board(std::cout);
    std::cout << std::endl;
  }
};

class ConsoleGameIO : public PrintCurrentBoard {
public:
  static
  NextColor
  get_next_color() {
    while (true) {
      std::string line;
      getline(std::cin, line);
      if (!std::cin) throw std::runtime_error("eof");

      switch (line[0]) {
      case 'r': return NextColor::RED; break;
      case 'b': return NextColor::BLUE; break;
      case 'w': return NextColor::WHITE; break;
      default: {
        std::cout << "not a valid color, try again" << std::endl;
        continue;
      }
      }
      break;
    }
  }

  ComputersResponse
  get_computers_response(const Board & board, PlayerMove pm, bool error) {
    (void) pm;
    (void) board;
    if (error) {
      std::cout << "Computer could not have moved there, try again" << std::endl;
    }
    std::cout << "Where did the computer move?" << std::endl;
    auto placement = get_card_placement_line(std::cin, false);

    std::cout << "What is the next Color?" << std::endl;
    auto next_color = get_next_color();

    return {placement, next_color};
  }
};

class VirtualGameIO : public PrintCurrentBoard {
public:
  ComputersResponse
  get_computers_response(const Board & board, PlayerMove pm, bool error) const {
    (void) error;
    static int counter_ = 0;
    assert(!error);
    auto toret = possible_computer_card_placements_post_shift(board, pm);
    auto placement = toret[std::max(0, std::rand()) % toret.size()];

    NextColor next_color;
    switch (counter_++ % 3) {
    case 0: next_color = NextColor::BLUE; break;
    case 1: next_color = NextColor::RED; break;
    case 2: next_color = NextColor::WHITE; break;
    default: assert(false); next_color = NextColor::WHITE;
    }

    return {placement, next_color};
  }
};

template<class GameIO>
void
run_game(Board board, GameIO gio) {
  while (true) {
    gio.current_board(board);

    if (game_is_over(board)) {
      throw std::runtime_error("game over!");
    }

    auto player_move = run_minimax(board_evaluator, board);

    board.shift(player_move);

    bool error = false;
    while (true) {
      auto cr = gio.get_computers_response(board, player_move, error);

      try {
        board.computers_move(player_move, cr.card_placement, cr.next_color);
      }
      catch (...) {
        error = true;
        continue;
      }

      break;
    }
  }
}

int
main(int argc, char *argv[]) {
  // get initial board state
  std::istream *is = nullptr;
  if (argc != 2) {
    std::cout << "Enter Initial State" << std::endl;
    is = &std::cin;
  }
  else {
    is = new std::ifstream(argv[1]);
  }

  auto board = read_board_from_human_input(*is);

  //  run_game(board, ConsoleGameIO());
  run_game(board, VirtualGameIO());

  return 0;
}

#endif
