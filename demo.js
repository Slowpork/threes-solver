var demo = {}

demo.load = function () {
    /* wrappers for our C code */
    var create_board = Module.cwrap('create_board', 'number', ['string']);
    var serialize_board = Module.cwrap('serialize_board', 'string', ['number']);
    var create_worker = Module.cwrap('create_worker', 'number', ['string']);
    var get_next_move = function (wh, board, on_move) {
        var fn_ptr;

        var new_on_move = function (player_move) {
            Runtime.removeFunction(fn_ptr);
            return on_move(Pointer_stringify(player_move));
        }

        fn_ptr = Runtime.addFunction(new_on_move);

        Module.ccall('get_next_move', 'number', ['number', 'number', 'number'],
                     [wh, board, fn_ptr]);
    };
    var serialize_board = function (board) {
        var BUF_SIZE = 256;
        buf = Runtime.stackAlloc(BUF_SIZE);
        amt = Module.ccall('serialize_board', 'number', ['number', 'number', 'number'],
                           [board, buf, BUF_SIZE]);
        if (amt == BUF_SIZE) throw "bad";
        return Pointer_stringify(buf, amt);
    };
    var make_computers_move = Module.cwrap('make_computers_move', 'number',
                                           ['number', 'string', 'number', 'number', 'number', 'string']);

    var shift_board = Module.cwrap('shift_board', 'number',
                                   ['number', 'string']);

    // -------------------------------------

    var stop_event = function (event) {
        if (event.stopPropagation) {
            event.stopPropagation();
        }
        event.preventDefault();
        return false;
    };

    var g_board;
    var g_wh = create_worker("threes-solver-worker.js");
    var g_last_players_move;

    var hide_all = function () {
        var PANES = ["enter_initial_board", "game_view", "board_view",
                     "computing_next_move", "enter_computers_response",
                     "command_view",
                     "next_color_view"];
        for (var i = 0; i < PANES.length; ++i) {
            document.getElementById(PANES[i]).style.display = 'none';
        }
    };

    var populate_board_view = function (board) {
        var serialized_board = serialize_board(board);
        var board_values = serialized_board.split(" ");
        document.getElementById("next_color_cell").textContent = board_values[0];

        for (var i = 0; i < 16; ++i) {
            var board_val = board_values[i + 1];
            if (board_val.trim() == "0") board_val = "_";
            document.getElementById("c" + i + "_cell").textContent = board_val;
        }
    }

    var switch_to_computing_next_move = function () {
        hide_all();
        populate_board_view(g_board);
        document.getElementById("board_title_cell").textContent = 'Current Board';
        document.getElementById("next_color_view").style.display = 'block';
        document.getElementById("game_view").style.display = 'block';
        document.getElementById("board_view").style.display = 'block';
        document.getElementById("computing_next_move").style.display = 'block';
    };

    var text_content_from_move = function (move) {
        if (move == "SWIPE_UP") return "SWIPE UP!";
        if (move == "SWIPE_DOWN") return "SWIPE DOWN!";
        if (move == "SWIPE_LEFT") return "SWIPE LEFT!";
        if (move == "SWIPE_RIGHT") return "SWIPE RIGHT!";
        throw Exception("what!");
    };

    var board_next_color = function (board) {
        return serialize_board(board).split(" ")[0].trim();
    };

    var max_card_value = function (board) {
        var board_values = serialize_board(board).split(" ");
        var max_val = 0;
        for (var i = 1; i < 1 + 16; ++i) {
            max_val = Math.max(parseInt(board_values[i]), max_val);
        }
        return max_val;
    }

    var switch_to_computers_response = function () {
        hide_all();

        var form = document.getElementById("enter_computers_response_form");
        var card_value_elt = form.card_value;
        form.reset();

        // deleting all children nodes
        while (card_value_elt.hasChildNodes()) {
            card_value_elt.removeChild(card_value_elt.lastChild);
        }

        var next_color = board_next_color(g_board)
        if (next_color == "blue" || next_color == "red") {
            var option_elt = document.createElement("option");
            option_elt.value = next_color == "blue" ? "1" : "2";
            option_elt.textContent = option_elt.value;
            card_value_elt.appendChild(option_elt);
            card_value_elt.selectedIndex = 0;
        }
        else {
            var values = [3];
            for (var i = 6; i < max_card_value(g_board); i *= 2) {
                values.push(i);
            }

            if (values.length != 1) {
                var option_elt = document.createElement("option");
                option_elt.textContent = "Select";
                card_value_elt.appendChild(option_elt);
            }

            for (var i = 0; i < values.length; ++i) {
                var option_elt = document.createElement("option");
                option_elt.value = values[i] + "";
                option_elt.textContent = option_elt.value;
                card_value_elt.appendChild(option_elt);
            }
            card_value_elt.selectedIndex = 0;
        }

        document.getElementById("game_view").style.display = 'block';
        document.getElementById("board_title_cell").textContent = 'Board after swipe';
        populate_board_view(g_board);
        document.getElementById("board_view").style.display = 'block';
        document.getElementById("enter_computers_response").style.display = 'block';
    };

    var switch_to_command_view = function (move) {
        hide_all();
        document.getElementById("move_to_make_cell").textContent = text_content_from_move(move);
        document.getElementById("command_view").style.display = 'block';
    }

    var on_next_move = function (move) {
        g_last_players_move = move;
        shift_board(g_board, move);
        switch_to_command_view(move);
    };

    var get_radio_value = function (form, name) {
        var radios = form[name]
        for (var i = 0; i < radios.length; ++i) {
            if (radios[i].checked) return radios[i].value;
        }
    };

    var handle_initial_board_submit = function (form, event) {
        var board_string = "";

        var next_color_value = get_radio_value(form, "next_color");
        if (!next_color_value) {
            alert("invalid board input!");
            return;
        }
        board_string += next_color_value + " ";

        for (var i = 0; i < 16; ++i) {
            var cell_value = form["c" + i].value.trim();
            if (cell_value == "") cell_value = "0";
            board_string += cell_value + " ";
        }

        g_board = create_board(board_string);
        if (g_board) {
            switch_to_computing_next_move();
            get_next_move(g_wh, g_board, on_next_move);
        }
        else {
            alert("invalid board input!");
        }
    };

    var handle_computers_response_submit = function (form, event) {
        var new_next_color_value = get_radio_value(form, "next_color");
        if (!new_next_color_value) {
            alert("choose a next color!!");
            return;
        }

        var new_card_value = parseInt(form.card_value.value);
        if (isNaN(new_card_value)) {
            alert("card value must be a number!");
            return;
        }

        var new_card_location = parseInt(get_radio_value(form, "card_location"));
        var new_card_x = new_card_location % 4;
        var new_card_y = new_card_location / 4;

        var valid_move = make_computers_move(g_board,
                                             g_last_players_move,
                                             new_card_value,
                                             new_card_x,
                                             new_card_y,
                                             new_next_color_value);
        if (!valid_move) {
            alert("invalid computer move!");
            return;
        }

        switch_to_computing_next_move();
        get_next_move(g_wh, g_board, on_next_move);
    };

    document
    .getElementById('initial_board_form')
    .addEventListener('submit', function (event) {
            try {
                handle_initial_board_submit(this, event);
            }
            catch (err) {
                alert("ERROR sorry :(");
            }
            return stop_event(event);
        });

    document
    .getElementById('enter_computers_response_form')
    .addEventListener('submit', function (event) {
            try {
                handle_computers_response_submit(this, event);
            }
            catch (err) {
                alert("ERROR sorry :(");
            }
            return stop_event(event);
        });

    document
    .getElementById('command_ack')
    .addEventListener('click', function (event) {
            switch_to_computers_response();
        });
};
