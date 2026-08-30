/*##########################

Move generator

#############################*/
#include "config.hpp" // IWYU pragma: keep
#include "ChessEngine.hpp"

//convert promoted piece to character
char promoted_pieces[128];

//print function for testing
void ChessEngine::print_attacked_squares(int attacking_side)
{
    cout<<"\n";
    for (int rank = 0; rank<8; rank++)
    {
        cout<<" "<<(8 - rank)<<"  ";
        for (int file = 0; file<8; file++)
        {
            // rank 8 is top most, chess board like normal 
            // counting from top left
            int square = 8*rank + file;
            cout<<is_square_attacked(square, attacking_side)<<" ";
        }
        cout<<"\n";
    }
    cout<<"    ";
    for (char file = 'a'; file<='h'; file++)
        cout<<file<<" ";
    cout<<"\n\n";
}