import urllib.request
import zipfile
import os
import chess
import chess.engine
import chess.pgn
import io

def get_stockfish():
    if not os.path.exists("stockfish_win.zip"):
        print("Downloading Stockfish...")
        url = "https://github.com/official-stockfish/Stockfish/releases/download/sf_16.1/stockfish-windows-x86-64-avx2.zip"
        urllib.request.urlretrieve(url, "stockfish_win.zip")
    
    if not os.path.exists("stockfish_dir"):
        print("Extracting Stockfish...")
        with zipfile.ZipFile("stockfish_win.zip", 'r') as zip_ref:
            zip_ref.extractall("stockfish_dir")

pgn_data = """[Event "rated bullet game"]
[Site "https://lichess.org/IlBINBVA"]
[Date "2026.09.03"]
[Round "-"]
[White "RadianceEngine"]
[Black "Domino_bot"]
[Result "1-0"]
[GameId "IlBINBVA"]
[UTCDate "2026.09.03"]
[UTCTime "13:13:01"]
[WhiteElo "2357"]
[BlackElo "1901"]
[WhiteRatingDiff "+1"]
[BlackRatingDiff "-6"]
[WhiteTitle "BOT"]
[BlackTitle "BOT"]
[Variant "Standard"]
[TimeControl "60+0"]
[ECO "D35"]
[Opening "Queen's Gambit Declined: Exchange Variation, Positional Variation"]
[Termination "Normal"]
[Annotator "lichess.org"]

1. d4 { [%clk 0:01:00] } 1... e6 { [%clk 0:01:00] } 2. c4 { [%clk 0:01:00] } 2... Nf6 { [%clk 0:00:58] } 3. Nc3 { [%clk 0:01:00] } 3... d5 { [%clk 0:00:56] } 4. cxd5 { [%clk 0:01:00] } 4... exd5 { [%clk 0:00:54] } 5. Bg5 { [%clk 0:01:00] } 5... Be7 { [%clk 0:00:52] } 6. e3 { [%clk 0:01:00] } 6... O-O { [%clk 0:00:50] } 7. Nf3 { [%clk 0:01:00] } 7... h6 { [%clk 0:00:48] } 8. Bh4 { [%clk 0:01:00] } 8... Be6 { [%clk 0:00:46] } 9. Bd3 { [%clk 0:01:00] } 9... Nc6 { [%clk 0:00:45] } 10. O-O { [%clk 0:00:58] } 10... Nb4 { [%clk 0:00:43] } 11. Bb1 { [%clk 0:00:56] } 11... c5 { [%clk 0:00:41] } 12. dxc5 { [%clk 0:00:54] } 12... g5 { [%clk 0:00:40] } 13. Bg3 { [%clk 0:00:52] } 13... Bxc5 { [%clk 0:00:38] } 14. Ne2 { [%clk 0:00:51] } 14... Ne4 { [%clk 0:00:37] } 15. a3 { [%clk 0:00:49] } 15... Nc6 { [%clk 0:00:35] } 16. Nc3 { [%clk 0:00:47] } 16... Nxc3 { [%clk 0:00:34] } 17. bxc3 { [%clk 0:00:46] } 17... Qe7 { [%clk 0:00:32] } 18. h4 { [%clk 0:00:44] } 18... g4 { [%clk 0:00:31] } 19. Nd4 { [%clk 0:00:43] } 19... Nxd4 { [%clk 0:00:30] } 20. exd4 { [%clk 0:00:41] } 20... Bd6 { [%clk 0:00:29] } 21. Re1 { [%clk 0:00:40] } 21... Rad8 { [%clk 0:00:28] } 22. Ra2 { [%clk 0:00:39] } 22... Bxg3 { [%clk 0:00:26] } 23. fxg3 { [%clk 0:00:37] } 23... Qxh4 { [%clk 0:00:25] } 24. gxh4 { [%clk 0:00:36] } 24... Bd7 { [%clk 0:00:24] } 25. Qd3 { [%clk 0:00:35] } 25... f5 { [%clk 0:00:23] } 26. Rae2 { [%clk 0:00:34] } 26... a6 { [%clk 0:00:22] } 27. Re5 { [%clk 0:00:33] } 27... Kg7 { [%clk 0:00:21] } 28. Ba2 { [%clk 0:00:32] } 28... Bb5 { [%clk 0:00:20] } 29. Qg3 { [%clk 0:00:31] } 29... Bc6 { [%clk 0:00:20] } 30. Bb1 { [%clk 0:00:30] } 30... Rfe8 { [%clk 0:00:19] } 31. Rxe8 { [%clk 0:00:29] } 31... Rxe8 { [%clk 0:00:18] } 32. Rxe8 { [%clk 0:00:28] } 32... Bxe8 { [%clk 0:00:17] } 33. Qe5+ { [%clk 0:00:27] } 33... Kf8 { [%clk 0:00:16] } 34. Qxf5+ { [%clk 0:00:26] } 34... Ke7 { [%clk 0:00:15] } 35. Qh7+ { [%clk 0:00:25] } 35... Kd6 { [%clk 0:00:14] } 36. Qxh6+ { [%clk 0:00:24] } 36... Kc7 { [%clk 0:00:13] } 37. Qg5 { [%clk 0:00:24] } 37... Bc6 { [%clk 0:00:12] } 38. Qxg4 { [%clk 0:00:23] } 38... b5 { [%clk 0:00:12] } 39. Qg5 { [%clk 0:00:22] } 39... Kc8 { [%clk 0:00:11] } 40. Kh2 { [%clk 0:00:21] } 40... Bd7 { [%clk 0:00:10] } 41. Bf5 { [%clk 0:00:21] } 41... Bxf5 { [%clk 0:00:09] } 42. Qxf5+ { [%clk 0:00:20] } 42... Kd8 { [%clk 0:00:07] } 43. Qe6 { [%clk 0:00:19] } 43... a5 { [%clk 0:00:07] } 44. Qxd5+ { [%clk 0:00:19] } 44... Kc7 { [%clk 0:00:06] } 45. Qxb5 { [%clk 0:00:18] } 45... Kd6 { [%clk 0:00:06] } 46. Qc5+ { [%clk 0:00:18] } 46... Kd7 { [%clk 0:00:06] } 47. Qxa5 { [%clk 0:00:17] } 47... Ke7 { [%clk 0:00:05] } 48. Qc5+ { [%clk 0:00:17] } 48... Kf7 { [%clk 0:00:05] } 49. Qe5 { [%clk 0:00:16] } 49... Kf8 { [%clk 0:00:05] } 50. Qc7 { [%clk 0:00:16] } 50... Kg8 { [%clk 0:00:04] } 51. g4 { [%clk 0:00:15] } 51... Kf8 { [%clk 0:00:03] } 52. g5 { [%clk 0:00:15] } 52... Kg8 { [%clk 0:00:03] } 53. g6 { [%clk 0:00:15] } 53... Kh8 { [%clk 0:00:02] } 54. Qh7# { [%clk 0:00:15] } 1-0"""

def analyze():
    get_stockfish()
    
    game = chess.pgn.read_game(io.StringIO(pgn_data))
    
    import glob
    engine_path = glob.glob("stockfish_dir/stockfish*/*.exe")[0]
    
    with chess.engine.SimpleEngine.popen_uci(engine_path) as engine:
        board = game.board()
        prev_score = 0
        mistakes = []
        
        info_start = engine.analyse(board, chess.engine.Limit(time=0.1))
        score_before = info_start["score"].white().score(mate_score=10000)
        
        for i, move in enumerate(game.mainline_moves()):
            is_black = i % 2 != 0
            
            board.push(move)
            
            # Eval after move
            info_after = engine.analyse(board, chess.engine.Limit(time=0.1))
            score_after = info_after["score"].white().score(mate_score=10000)
            
            # For Black, a mistake is when score goes UP (better for white)
            if is_black:
                eval_diff = score_after - score_before
                if eval_diff > 150:  # loss of 1.5+ pawns
                    mistakes.append((board.ply(), move, score_before, score_after, eval_diff))
            
            score_before = score_after
            
        print("MISTAKES BY DOMINO_BOT (Black):")
        for ply, move, sb, sa, diff in mistakes:
            move_num = (ply + 1) // 2
            print(f"Move {move_num}... {move}: Eval changed from {sb/100.0:+.2f} to {sa/100.0:+.2f} (Loss of {diff/100.0:.2f} pawns)")

if __name__ == "__main__":
    analyze()
