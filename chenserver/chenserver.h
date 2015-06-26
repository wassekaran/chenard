/*
    chenserver.h  -  Don Cross  -  http://cosinekitty.com

    Main header file for server version of Chenard chess engine.
*/

#include <assert.h>
#include <time.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#include <WinSock2.h>
#include "chess.h"
#include "uiserver.h"

class ChessGameState;

enum MoveFormatKind
{
    MOVE_FORMAT_INVALID,
    MOVE_FORMAT_ALGEBRAIC,
    MOVE_FORMAT_PGN,
};

inline bool StartsWith(const std::string& text, const std::string& prefix)
{
    return
        (text.length() >= prefix.length()) &&
        (0 == text.compare(0, prefix.length(), prefix));
}

inline bool EndsWith(const std::string& text, const std::string& suffix)
{
    return
        (text.length() >= suffix.length()) &&
        (0 == text.compare(text.length() - suffix.length(), suffix.length(), suffix));
}

void PrintUsage();
MoveFormatKind ParseFormatArg(const std::vector<std::string>& args, size_t& index);     // use if other args may follow format
bool ParseFormatArg(const std::vector<std::string>& args, std::string& errorToken, MoveFormatKind& format);     // use if format is only arg
std::string DualMoveFormatResponse(ChessGameState& game, Move move);
std::string ExecuteCommand(ChessGameState& game, ChessUI_Server& ui, const std::string& command, bool& keepRunning);
std::string GameStatus(ChessGameState& game);      // Forsyth Edwards Notation
std::string MakeMoves(ChessGameState& game, const std::vector<std::string>& moveTokenList);
std::string LegalMoveList(ChessGameState& game, const std::vector<std::string>& args);
std::string TestLegality(ChessGameState& game, const std::string& notation);
std::string Think(ChessUI_Server& ui, ChessGameState& game, int thinkTimeMillis);
std::string Undo(ChessGameState& game, int numTurns);
std::string History(ChessGameState& game, const std::vector<std::string>& args);

const size_t LONGMOVE_MAX_CHARS = 6;        // max: "e7e8q\0" is 6 characters
bool FormatLongMove(bool whiteToMove, Move move, char notation[LONGMOVE_MAX_CHARS]);
char SquareCharacter(SQUARE);

class ChessGameState
{
public:
    ChessGameState() {}

    void Reset();
    const char *GameResult();
    std::string GetForsythEdwardsNotation();
    std::string FormatAlg(Move move);      // caller must pass only verified legal moves
    std::string FormatPgn(Move move);      // caller must pass only verified legal moves
    std::string Format(Move move, MoveFormatKind format);       // caller must pass only verified legal moves, and must pass valid format specifier
    std::vector<std::string> History(MoveFormatKind format);
    bool ParseMove(const std::string& notation, Move& move);    // returns true only if notation represents legal move
    int NumTurns() const { return static_cast<int>(moveStack.size()); }
    void PushMove(Move move);   // caller must pass only verified legal moves
    void PopMove();
    int GenMoves(MoveList& ml) { return board.GenMoves(ml); }
    bool Think(ChessUI_Server& ui, int thinkTimeMillis, Move& move);
    bool IsGameOver() { return board.GameIsOver(); }

private:
    ChessGameState(const ChessGameState&);              // disable copy constructor
    ChessGameState& operator=(const ChessGameState&);   // disable assignment operator

    struct MoveState
    {
        Move move;
        UnmoveInfo unmove;

        MoveState(const Move& _move, const UnmoveInfo& _unmove)
            : move(_move)
            , unmove(_unmove)
        {}
    };

    ChessBoard board;
    std::vector<MoveState> moveStack;
};

/*
    ChessCommandInterface is an abstract class representing
    a way of sending and receiving strings of text commands.
*/
class ChessCommandInterface
{
public:
    virtual ~ChessCommandInterface() {}
    virtual bool ReadLine(std::string& line, bool& keepRunning) = 0;
    virtual void WriteLine(const std::string& line) = 0;
};

class ChessCommandInterface_stdio : public ChessCommandInterface
{
public:
    ChessCommandInterface_stdio();
    virtual ~ChessCommandInterface_stdio();
    virtual bool ReadLine(std::string& line, bool& keepRunning);
    virtual void WriteLine(const std::string& line);

private:
    ChessCommandInterface_stdio(const ChessCommandInterface_stdio&);                // disable copy constructor
    ChessCommandInterface_stdio& operator= (const ChessCommandInterface_stdio&);    // disable assignment
};

class ChessCommandInterface_tcp : public ChessCommandInterface
{
public:
    explicit ChessCommandInterface_tcp(int _port);
    virtual ~ChessCommandInterface_tcp();
    virtual bool ReadLine(std::string& line, bool& keepRunning);
    virtual void WriteLine(const std::string& line);

private:
    const int port;
    bool initialized;
    bool ready;
    SOCKET hostSocket;
    SOCKET clientSocket;
    enum data_mode { MODE_LINE, MODE_HTTP } mode;
    char httpMinorVersion;

    ChessCommandInterface_tcp(const ChessCommandInterface_tcp&);                // disable copy constructor
    ChessCommandInterface_tcp& operator= (const ChessCommandInterface_tcp&);    // disable assignment

    void CloseSocket(SOCKET &sock)
    {
        if (sock != INVALID_SOCKET)
        {
            ::closesocket(sock);
            sock = INVALID_SOCKET;
        }
    }

    std::string UrlDecode(const std::string& urltext);
};
