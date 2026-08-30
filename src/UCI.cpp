/*##########################

UCI functions

#############################*/
#include "config.hpp"
#include "ChessEngine.hpp"

#include <chrono>

// Instantiate the engine used for CLI
ChessEngine uci_engine;

//get time in milisecs
long long get_time_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

//parse user or GUI move string for UCI purpose (a2a3, a7a8q)
int ChessEngine::parse_move_string(const string& move_string)
{
    //we need to check if move provided by the user/GUI is legal
    //so create a move list and check if that move is present in it

    MoveList move_list;
    generate_moves(move_list);

    //parse the move rank = 8 - board rank
    //square = 8*rank + file = 8*(8-board_rank) + file
    int source_square = 8*(8 - (move_string[1] - '0')) + (move_string[0] - 'a');
    int target_square = 8*(8 - (move_string[3] - '0')) + (move_string[2] - 'a');
    //check if provided move has promotion
    int ind = 0;
    while(move_string[ind]!='\0') {ind++;}
    bool has_promotion  = ((ind>=5)?true:false);
    //iterate over the list to check if this move is present
    for (int i=0;i<move_list.index;i++)
    {
        int move = move_list.moves[i];
        //if soruce and target match
        if (decode_move_source(move)==source_square && decode_move_target(move)==target_square)
        {            
            //if promotion is involved
            if (has_promotion)
            {
                //prmotion piece matches
                if (promoted_pieces[decode_move_promo_piece(move)] == move_string[4])
                    return move;
            }
            //move matched
            else
            {
                //if the move from move list has promotion skip
                if (decode_move_promo_piece(move)) continue;
                
                //legal move
                return move;
            }
        }
    }

    //illegal move
    return 0;
}

//parse "position" command for UCI (examples of this command are in notion doc)
void parse_position(const string &command)
{
    //cuurent index set after "position"
    int ind = 9;
    //stores index of moves keyword, using size_t because it is the right thing to do and also becuase we will be comparing with string::npos which is size_t type (unsigned int basically) and 2^64 -1, int is only 2^63 - 1
    size_t pos = string::npos;
    //avoid researching for moves keyword
    bool searched_for_moves = false;
    //if startpos is present
    if (command.compare(ind, 8, "startpos")==0)
    {
        //initialize starting position
        uci_engine.parse_FEN_string(start_position);
    }
    //fen is present
    else
    {
        //if fen not present by mistake
        if (command[ind]!='f') {uci_engine.parse_FEN_string(start_position);}
        //fen present
        else
        {
            //first character of fen
            ind+= 4; 
            //check if moves keyword present
            pos = command.find('m');
            searched_for_moves = true;
            //if moves keyword found fen string is from ind till pos - 2
            if (pos!=string::npos)
                uci_engine.parse_FEN_string(command.substr(ind, pos-2-ind+1));
            //if moves keyword not found, take till last
            else
                uci_engine.parse_FEN_string(command.substr(ind));
        }
    }
    //if we havent searched for moves keyword in above code yet
    if (!searched_for_moves)
        pos = command.find('m');
    //if moves keyword found
    if (pos!=string::npos)
    {
        ind = pos;
        //shift index to first move
        ind+= 6;
        int n = command.size();
        while(ind<n)
        {
            int move = 0;
            //move does not have promotion
            if (command[ind+4]==' '|| ind+4==n)
                {move = uci_engine.parse_move_string(command.substr(ind, 4)); ind+=5;} //parse and shift index to next move
            //move has promotion
            else
                {move = uci_engine.parse_move_string(command.substr(ind, 5)); ind+=6;} //parse and shift index to next move
            
            //illegal move
            if (move==0)
                break;
            
            //make the move on the chess board
            uci_engine.make_move(move, all_moves);
        }
    }
    //debugging purpose
    uci_engine.print_board();
}

extern long long search_time_limit;

//parse "go" commaand for UCI
void parse_go(const string &command)
{
    int depth = -1;
    long long time_to_search = -1;

    size_t depth_pos = command.find("depth");
    if (depth_pos != string::npos) {
        int ind = depth_pos + 6;
        depth = 0;
        while(ind < command.size() && command[ind] >= '0' && command[ind] <= '9') {
            depth = depth * 10 + (command[ind] - '0');
            ind++;
        }
    }

    size_t movetime_pos = command.find("movetime");
    if (movetime_pos != string::npos) {
        int ind = movetime_pos + 9;
        time_to_search = 0;
        while(ind < command.size() && command[ind] >= '0' && command[ind] <= '9') {
            time_to_search = time_to_search * 10 + (command[ind] - '0');
            ind++;
        }
    }

    size_t wtime_pos = command.find("wtime");
    size_t btime_pos = command.find("btime");
    if (wtime_pos != string::npos && btime_pos != string::npos && time_to_search == -1) {
        int wtime = 0;
        int ind = wtime_pos + 6;
        while(ind < command.size() && command[ind] >= '0' && command[ind] <= '9') {
            wtime = wtime * 10 + (command[ind] - '0');
            ind++;
        }

        int btime = 0;
        ind = btime_pos + 6;
        while(ind < command.size() && command[ind] >= '0' && command[ind] <= '9') {
            btime = btime * 10 + (command[ind] - '0');
            ind++;
        }

        if (uci_engine.side == white) {
            time_to_search = wtime / 30;
        } else {
            time_to_search = btime / 30;
        }
    }

    if (depth == -1) {
        depth = 64; // Default to searching as deep as possible if no depth provided
    }
    
    // Set engine limits
    if (time_to_search != -1) {
        uci_engine.search_time_limit = time_to_search;
    } else {
        uci_engine.search_time_limit = 4500; // default to 4.5s if no time control provided
    }
    uci_engine.time_stopped = false;

    //search position
    uci_engine.search_position(depth);
}

//main UCI loop to connect with GUI
void uci_loop()
{
    //some commands for debugging
    cout << "\nCommands for debugging:\n";
    cout << "'d' - print current board position\n";

    //run loop unless broken
    while(true)
    {
        //user input
        string input;

        //flush output since i am using "\n" everywhere
        cout.flush();

        //take user input
        getline(cin, input);

        //check if input is there or not
        if (input.empty())
            continue;
        
        //parse debug "d" command
        if (input=="d")
            uci_engine.print_board();
        //GUI say isready, engine should say readyok
        else if (input.substr(0, 7)=="isready")
        {
            cout<<"readyok\n"; continue;
        }
        //parse position command
        else if (input.substr(0, 8)=="position")
        {
            parse_position(input);
        }
        //parse UCI ucinewgame command
        else if (input.substr(0, 10)=="ucinewgame")
        {
            //send command for starting position
            parse_position("position startpos");
        }
        //parse "uci" command, first check for ucinewgames, otherqise ucinewgames triggers this
        else if (input.substr(0, 3) == "uci")
        {
            // print engine info
            cout << "id name Domino"<< "\n";
            cout << "id author Dhiraj \n";
            cout << "uciok\n";
        }
        //parse go command
        else if (input.substr(0,2)=="go")
        {
            parse_go(input);
        }
        //parse quit command
        else if (input.substr(0,4)=="quit")
        {
            //exit
            break;
        }
    }
}