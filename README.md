threes-solver
=============

threes-solver is an implementation of a very basic AI for the
iPhone game [Threes](http://asherv.com/threes/). [Try it!](http://rianhunter.github.io/threes-solver/)

Why did you do this?
-----------------

I've been trying to get higher than 768 in Threes using a basic strategy for
a couple of days without success. I eventually got bored and thought it
would make more sense write a program to beat Threes. I haven't done much
AI programming since college. Later on, it was cool to try emscripten out
to port this code for the web.

How does it work?
-----------------

threes-solver uses a basic algorithm to select a "move" when
competing against another player. This algorithm is called [Minimax](http://en.wikipedia.com/Minimax)
and is likely taught in every undergraduate CS program.

For those of you learning (like me!), here's a introductory description
of how it works:

### Games as Trees

First you have to model the game a series of "states" and "transitions"
between those states.

A game state is basically every piece of
information that a player can observe about the game. E.g. in Chess
this would the locations of every piece on board. In my model of Threes
the state is the locations and type of all the cards on the board and
the "next" color shown at the top.

A transition is a valid move that a player can make that would take
the game from one state to another. Again, in Chess this would be like
moving a bishop piece diagonally some amount of spaces to another location
on the board. In Threes there are two types of moves, a player's move
and the computer's move. A player's move is just a swipe in any direction.
A computer's move is placing a card somewhere on the perpendicular line
on the opposite edge of the swipe.

Once you have data structues that encode the game state and code that
implements the valid transitions from one game state to another, you can
then use Minimax to select a good move.

Conceptually, Minimax maps out all the reachable game states from the
current state as a tree-like structure. Like in the picture below. Each
point in this "tree" represents a game state and each line represents a
transition from one game state to another. Sometimes the relationship
between two connected points is compared to a "parent" and "child," where
the parent is the earlier game state in the relationship.

<img src="tree.jpg?raw=true" width="500">

Each single line from a parent to a child represents one turn
of the game. Each path down the tree represents one possible way the game
could play out, it's just a sequence of turns. In most games there are
thousands of possible turn sequences just after three or four turns.
The number of possible paths from any single game state is in some ways a
measure of the difficulty of a game.

### Minimax Theory & Operation

Now one simple way to write an AI would be to visit every possible path from
your current game state and select the move that has the most and best
winning paths stemming from it. Minimax is more conservative, instead it chooses
the move that ensures minimum loss (or win from your opponent's POV).

For example, let's say you have the choice of flipping two coins. If you
flip coin A and get heads you win $10 but if you get tails you lose $5.
If you flip coin B the same but instead it's $1 and $0.50. In this situation Minimax
would choose to flip coin B because it's the choice with the minimum
possible loss, i.e. you only stand to lose $0.50 with coin B, even
though with coin A you could win $5 (and even despite the expected
winnings once choosing coin A are higher, $2.50).

Instead of choosing moves that will may help you win, Minimax chooses moves
that make it harder for your opponent to win. It's a defensive strategy:
it assumes that your opponent is the best possible opponent, or in
the preceding example, it assumes that coin is rigged. If this assumption
is false, it does no worse. You might observe that if you had a probalistic
model of your opponent, you could do better than vanilla Minimax.

An important thing to remember is that Minimax only works on so-called "zero-sum"
games, i.e. these are games where the amount you win is equal to the amount
your opponent loses. In a zero-sum game minimizing your maximum loss
is the same as maximizing your minimum win.

Minimax gets it name because the algorithm is structured as a series of alternating
minimum and maximum calls over game states as you traverse the game tree downward.
The intuition is this: to find the minimum loss for a given game state, first you need
to find the maximum possible loss for each move and choose the minimum.
The inverse works too: to find the maximum loss for a given game state,
first find the minimum possible loss after each move and choose the maximum.
This picture illustrates an example of the algorithm in action, note that
it starts with your opponent looking for the maximum loss.

<img src="minimax.png?raw=true" width="500">

The reason for the alternating minimums and maximums is that each player
is assuming the worst of the other but the best of themselves. In reality,
the opposing player isn't ensuring your maximum loss but instead their minimum
loss. In a zero-sum game it's the same thing.

One last thing, because Minimax is concerned with finding the minimum loss, it's
a good fit for solving Threes since you really can't win. There is only losing.

### Heuristics

The problem with Minimax is that for complex games (games with many possible
moves at any given game state) computing minimax can take a very long time. The amount of time
it takes is on the order of the the number of possible turn sequences leading up
to the game's end. For games like tic-tac-toe, this isn't a problem but for
games like Threes or Chess it's not practical to do a full run of minimax each turn.

One solution is to stop recursively computing the minimum or maxiumum loss after
some depth and instead approximate it. This ensures an upper bound on the amount
of game states Minimax has to visit. The alternative function you use to
compute this approximate value is called a heuristic.

Since Minimax is a recursive algorithm, it's important that your heuristic approximates
what Minimax would return at that level with high accuracy. If not, the algorithm will
break down. If it's a bad heuristic, you'll be computing the minimum maximum value of
a bad heuristic, which has no meaningful relation to winning or losing.

Your heuristic is like a compass guiding you to victory. With a perfectly accurate heuristic
you don't need large search depth. With the least accurate heuristic,
there is no search depth large enough that will ensure victory. That's unless your heurstic
is good enough to at least distinguish end game states and the final tallies.

Choosing an optimal heuristic is a bit of a black art. It requires a good understanding
of the game.

### Our Heuristic

A basic heuristic is used in threes-solver. It's based on the observation that game states
where all adjacent cards are close in value usually lead to getting cards at least as high
as 768.

The heuristic computes a valid called "friction" for a board state. First let's define
a friction value for adjacent cards:

    friction(1, 1) = 2
    friction(2, 2) = 2
    friction(1, 2) = 0
    friction(2, 1) = 0
    friction(1, b) = 1 + log_base_2(b / 3)
    friction(2, b) = 1 + log_base_2(b / 3)
    friction(a, b) = abs(log_base_2(b / 3) - log_base_2(a / 3))

The card friction function essentially returns 0 when the cards can be shifted over each other
and a higher value as the difference in values of the cards rises.

To find the board friction, we sum the card friction over every adjacent card pair both vertically
and horizontally. For example, for the friction is 8 in board below:

    12 6 3 2
     6 3 _ _
     1 _ _ _
     _ _ _ _

After we compute the friction we find the highest card value on the board. At the end this results
in this heuristic:

    heuristic(board) = board_friction(board) / max_card_value(board)

In other words we approximate that losing is proportional to board friction
but inversely proportional to max_card_value. Is this right? Kind of, read
the next section.

Does it actually solve Threes?
------------------------------

It does and it doesn't. The solver can reasonably reach card values of 768 but it is
not able to get higher than that in my testing. You might find that funny since the
entire reason I wrote the solver in the first place was to reach a card higher than 768.

There's a reason it only gets to 768. It's my heuristic function. It's exactly
the same heuristic I use when I'm playing in real life. It's no wonder I was only
ever able to get to 768.

You might even wonder if maybe the game designers of Threes designed it so it wasn't
possible to get higher than 768. I think they did, in a way.
I think they realized most people would mentally use a heuristic function similar
to mine and purposefully designed the game so that you would need a more complex heuristic
to reach higher values. I know
of [people who have claimed to reach 6144](http://forums.toucharcade.com/showthread.php?t=218248&page=42)
and they talk about the necessity of the computer playing the large pieces and keeping
a "diagonal" formation (not the horizontal formation guided by my heuristic).

At this point I've had my fun. Without spending
a lot more time learning the arcane game mechanics of Threes', I'll probably only
ever reach 768. That's fine with me, I've learned what I wanted to learn and prove
to myself that I could implement this. Also had some fun getting C++ running in the
browser.

Thanks
------

Thanks goes to the people who created Threes, including
[Asher Vollmer](http://asherv.com/threes/). It's clear they put a lot of time
and effort into making it a lasting and fun puzzle game. It's a way better game than 2048.

Copyright
---------

Copyright (C) 2014 Rian Hunter <rian@alum.mit.edu>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
