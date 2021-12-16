#pragma once

#include <Dymatic.h>
#include "Dymatic/Core/Base.h"

namespace Dymatic::Sandbox {

	struct Agent
	{
		glm::vec2 position;
		float angle;
		glm::vec4 color;

		Agent(glm::vec2 position, float angle, glm::vec4 color = glm::vec4(1.0f))
			: position(position), angle(angle), color(color)
		{
		}
	};

	class AgentSimulation
	{
	public:
		AgentSimulation();
		void Update(Timestep ts);
	private:
		int hash(int state)
		{
			state ^= 2747636419;
			state *= 2654435769;
			state ^= state >> 16;
			state *= 2654435769;
			state ^= state >> 16;
			state *= 2654435769;
			return state;
		}

		float sense(Agent agent, float sensorAngleOffset);
	private:

		int m_Seed = 12;

		std::vector<Agent> m_Agents;
		int scaleFactor = 1;
		int m_Width = 100 * scaleFactor;
		int m_Height = 100 * scaleFactor;
		glm::vec4 m_TrailMap[100][100];
		Ref<Texture2D> m_Trail;

		float m_MoveSpeed = 10.0f;
		float m_EvaporateSpeed = 0.025f / scaleFactor;
		float m_DiffusedSpeed = 1.0f;

		float m_SensorOffsetDst = 80.0f;
		float m_SensorSize = 10.0f;
		float m_SensorAngleSpacing = 45.0f;
		float m_TurnSpeed = 5.0f;
	};

	class MandelbrotSet
	{
	public:
		MandelbrotSet();
		void OnImGuiRender();

		void DrawSetBuffer();

		double mapToReal(int x, int imageWidth, double minR, double maxR);
		double mapToImaginary(int y, int imageHeight, double minI, double maxI);

		int findMandelbrot(double cr, double ci, int max_iterations);

		void HSVtoRGB(float H, float S, float V, float* outR, float* outG, float* outB);
	private:
		Ref<Texture2D> m_MandelbrotTexture;


		int m_Width = 1080;
		int m_Height = 1080;

		float m_Zoom = 1.0f;

		float m_XPos = 0.0f;
		float m_YPos = 0.0f;

		float m_MoveSpeed = 0.1f;
		float m_ZoomSpeed = 0.1f;
		int m_Iterations = 50; //256 good

		float minR = -1.5, maxR = 0.0, minI = -1.0, maxI = 1.0;
		bool m_ZoomUniform = true;
	};

	struct SandParticle
	{
		int id = -1;
		float lifetime = 0.0f;
		glm::vec2 velocity = glm::vec2(0.0f);
		glm::vec4 color = glm::vec4(1.0f);
		bool updated = false;

		SandParticle();
		SandParticle(int id)
			: id(id)
		{
		}
	};

	class SandSimulation
	{
	public:
		SandSimulation();
		void OnImGuiRender();
	private:
		int GetNextID() { m_NextID++; return m_NextID; }
	private:

		Ref<Texture2D> m_SimulationTexture;
		int m_Width = 512;
		int m_Height = 512;

		int m_NextID = 0;

		SandParticle m_SandParticles[512][512] = { SandParticle(m_NextID) };
	};

	//Cloth Simulation

	struct Point
	{
		unsigned int id;
		glm::vec2 position, prevPosition;
		bool locked;
		Point(unsigned int id, glm::vec2 position, bool locked)
			: id(id), position(position), prevPosition(position), locked(locked)
		{}
	};

	struct Stick
	{
		unsigned int id;
		Point* pointA;
		Point* pointB;
		float length;
		Stick(unsigned int id, Point* a, Point* b, float length)
			: id(id), pointA(a), pointB(b), length(length)
		{}
	};

	class RopeSimulation
	{
	public:
		RopeSimulation();
		void OnImGuiRender(Timestep ts);
		void Simulate(Timestep ts);
	private:
		inline unsigned int GetNextId() { m_NextId++; return m_NextId; }
		void GenerateGrid();
		void GenerateTree();
		void TreeLoop(Point* point);

		int hash(int state)
		{
			state ^= 2747636419;
			state *= 2654435769;
			state ^= state >> 16;
			state *= 2654435769;
			state ^= state >> 16;
			state *= 2654435769;
			return state;
		}

	private:
		std::vector<Point> m_Points;
		std::vector<Stick> m_Sticks;
		int m_InterationNumber = 5;
		bool m_Simulating = false;
		unsigned int m_NextId = 0;
		Point* joinPoint = nullptr;
		Point* m_PreviousDrawPoint = nullptr;
		float m_CuttingTollerance = 2.0f;
		bool m_Floor = false;
		float m_FloorPos = 0.0f;
		float m_WindowWidth = 0.0f;
		int m_TreeSeed = 0;
		float m_BreakingPoint = 300.0f;
	};

	// Chess Simulation
	
	//struct Piece
	//{
	//	static const int None = 0;
	//	static const int King = 1;
	//	static const int Pawn = 2;
	//	static const int Knight = 3;
	//	static const int Bishop = 4;
	//	static const int Rook = 5;
	//	static const int Queen = 6;
	//
	//	static const int White = 8;
	//	static const int Black = 16;
	//
	//	static const int typeMask = 0b00111;
	//	static const int blackMask = 0b10000;
	//	static const int whiteMask = 0b01000;
	//	static const int colourMask = whiteMask | blackMask;
	//
	//	static bool IsColour(int piece, int colour) {
	//		return (piece & colourMask) == colour;
	//	}
	//
	//	static int Colour(int piece)
	//	{
	//		return piece & colourMask;
	//	}
	//
	//	static int PieceType(int piece)
	//	{
	//		return piece & typeMask;
	//	}
	//
	//	static bool IsRookOrQueen(int piece)
	//	{
	//		return (piece & 0b110) == 0b110;
	//	}
	//
	//	static bool IsBishopOrQueen(int piece)
	//	{
	//		return (piece & 0b101) == 0b101;
	//	}
	//
	//	static bool IsSlidingPiece(int piece)
	//	{
	//		return (piece & 0b100) != 0;
	//	}
	//};
	//
	//struct Move
	//{
	//	const int StartSquare;
	//	const int TargetSquare;
	//};
	//
	//class Board
	//{
	//public:
	//	Board()
	//	{
	//		Square = new int[64];
	//		for (int i = 0; i < 64; i++) Square[i] = 0;
	//
	//		LoadPositionFromFen(startFEN);
	//	}
	//
	//	~Board()
	//	{
	//		delete[] Square;
	//	}
	//	void LoadPositionFromFen(std::string fen);
	//
	//	int* Square;
	//	const std::string startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	//
	//	int ColorToMove = Piece::White;
	//};
	//
	//class ChessAI
	//{
	//public:
	//	ChessAI();
	//
	//	void OnImGuiRender(Timestep ts);
	//
	//	void OnEvent(Event& e);
	//private:
	//	bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
	//	bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);
	//
	//	Ref<Texture2D> GetPieceTexture(int piece);
	//
	//	// Chess Calculations
	//	void PrecomputedMoveData();
	//	//std::vector<Move> GenerateMoves();
	//
	//	inline bool IsWhite(int piece) { piece < 16; }
	//private:
	//	Board m_Board;
	//
	//	bool m_StartDrag = false;
	//	bool m_StopDrag = false;
	//	int32_t m_DragIndex = -1;
	//
	//	// Texture
	//	Ref<Texture2D> m_TextureWKing;
	//	Ref<Texture2D> m_TextureWPawn;
	//	Ref<Texture2D> m_TextureWKnight;
	//	Ref<Texture2D> m_TextureWBishop;
	//	Ref<Texture2D> m_TextureWRook;
	//	Ref<Texture2D> m_TextureWQueen;
	//
	//	// Chess Data
	//	int DirectionOffsets[8] = { 8, -8, -1, 1, 7, -7, 9, -9 };
	//	int NumSquaresToEdge[64][8];
	//
	//	std::vector<Move> moves;
	//};

	// Chess AI Build Up

	//namespace Chess
	//{
	//
	//	// BitBoard Utility
	//	static class BitBoardUtility {
	//	public:
	//		static bool ContainsSquare(unsigned long bitboard, int square) {
	//			return ((bitboard >> square) & 1) != 0;
	//		}
	//	};
	//
	//	static int WhiteIndex = 0;
	//	static int BlackIndex = 1;
	//
	//	// Board
	//	class Board {
	//	public:
	//		//const int WhiteIndex = 0;
	//		//const int BlackIndex = 1;
	//
	//		// Stores piece code for each square on the board.
	//		// Piece code is defined as piecetype | colour code
	//		int Square[64];
	//
	//		bool WhiteToMove;
	//		int ColourToMove;
	//		int OpponentColour;
	//		int ColourToMoveIndex;
	//
	//		// Bits 0-3 store white and black kingside/queenside castling legality
	//		// Bits 4-7 store file of ep square (starting at 1, so 0 = no ep square)
	//		// Bits 8-13 captured piece
	//		// Bits 14-... fifty mover counter
	//		Stack<uint> gameStateHistory;
	//		uint32_t currentGameState;
	//
	//		int plyCount; // Total plies played in game
	//		int fiftyMoveCounter; // Num ply since last pawn move or capture
	//
	//		unsigned long ZobristKey;
	//		/// List of zobrist keys 
	//		public Stack<unsigned long> RepetitionPositionHistory;
	//
	//		int KingSquare[2]; // index of square of white and black king
	//
	//		PieceList[] rooks;
	//		PieceList[] bishops;
	//		PieceList[] queens;
	//		PieceList[] knights;
	//		PieceList[] pawns;
	//
	//		PieceList[] allPieceLists;
	//
	//		const uint32_t whiteCastleKingsideMask = 0b1111111111111110;
	//		const uint32_t whiteCastleQueensideMask = 0b1111111111111101;
	//		const uint32_t blackCastleKingsideMask = 0b1111111111111011;
	//		const uint32_t blackCastleQueensideMask = 0b1111111111110111;
	//
	//		const uint32_t whiteCastleMask = whiteCastleKingsideMask & whiteCastleQueensideMask;
	//		const uint32_t blackCastleMask = blackCastleKingsideMask & blackCastleQueensideMask;
	//
	//		PieceList GetPieceList(int pieceType, int colourIndex) {
	//			return allPieceLists[colourIndex * 8 + pieceType];
	//		}
	//
	//		// Make a move on the board
	//		// The inSearch parameter controls whether this move should be recorded in the game history (for detecting three-fold repetition)
	//		void MakeMove(Move move, bool inSearch = false) {
	//			uint32_t oldEnPassantFile = (currentGameState >> 4) & 15;
	//			uint32_t originalCastleState = currentGameState & 15;
	//			uint32_t newCastleState = originalCastleState;
	//			currentGameState = 0;
	//
	//			int opponentColourIndex = 1 - ColourToMoveIndex;
	//			int moveFrom = move.StartSquare;
	//			int moveTo = move.TargetSquare;
	//
	//			int capturedPieceType = Piece.PieceType(Square[moveTo]);
	//			int movePiece = Square[moveFrom];
	//			int movePieceType = Piece.PieceType(movePiece);
	//
	//			int moveFlag = move.MoveFlag;
	//			bool isPromotion = move.IsPromotion;
	//			bool isEnPassant = moveFlag == Move.Flag.EnPassantCapture;
	//
	//			// Handle captures
	//			currentGameState |= (unsigned short)(capturedPieceType << 8);
	//			if (capturedPieceType != 0 && !isEnPassant) {
	//				ZobristKey ^= Zobrist.piecesArray[capturedPieceType, opponentColourIndex, moveTo];
	//				GetPieceList(capturedPieceType, opponentColourIndex).RemovePieceAtSquare(moveTo);
	//			}
	//
	//			// Move pieces in piece lists
	//			if (movePieceType == Piece.King) {
	//				KingSquare[ColourToMoveIndex] = moveTo;
	//				newCastleState &= (WhiteToMove) ? whiteCastleMask : blackCastleMask;
	//			}
	//			else {
	//				GetPieceList(movePieceType, ColourToMoveIndex).MovePiece(moveFrom, moveTo);
	//			}
	//
	//			int pieceOnTargetSquare = movePiece;
	//
	//			// Handle promotion
	//			if (isPromotion) {
	//				int promoteType = 0;
	//				switch (moveFlag) {
	//				case Move.Flag.PromoteToQueen:
	//					promoteType = Piece.Queen;
	//					queens[ColourToMoveIndex].AddPieceAtSquare(moveTo);
	//					break;
	//				case Move.Flag.PromoteToRook:
	//					promoteType = Piece.Rook;
	//					rooks[ColourToMoveIndex].AddPieceAtSquare(moveTo);
	//					break;
	//				case Move.Flag.PromoteToBishop:
	//					promoteType = Piece.Bishop;
	//					bishops[ColourToMoveIndex].AddPieceAtSquare(moveTo);
	//					break;
	//				case Move.Flag.PromoteToKnight:
	//					promoteType = Piece.Knight;
	//					knights[ColourToMoveIndex].AddPieceAtSquare(moveTo);
	//					break;
	//
	//				}
	//				pieceOnTargetSquare = promoteType | ColourToMove;
	//				pawns[ColourToMoveIndex].RemovePieceAtSquare(moveTo);
	//			}
	//			else {
	//				// Handle other special moves (en-passant, and castling)
	//				switch (moveFlag) {
	//				case Move.Flag.EnPassantCapture:
	//					int epPawnSquare = moveTo + ((ColourToMove == Piece.White) ? -8 : 8);
	//					currentGameState |= (ushort)(Square[epPawnSquare] << 8); // add pawn as capture type
	//					Square[epPawnSquare] = 0; // clear ep capture square
	//					pawns[opponentColourIndex].RemovePieceAtSquare(epPawnSquare);
	//					ZobristKey ^= Zobrist.piecesArray[Piece.Pawn, opponentColourIndex, epPawnSquare];
	//					break;
	//				case Move.Flag.Castling:
	//					bool kingside = moveTo == BoardRepresentation.g1 || moveTo == BoardRepresentation.g8;
	//					int castlingRookFromIndex = (kingside) ? moveTo + 1 : moveTo - 2;
	//					int castlingRookToIndex = (kingside) ? moveTo - 1 : moveTo + 1;
	//
	//					Square[castlingRookFromIndex] = Piece.None;
	//					Square[castlingRookToIndex] = Piece.Rook | ColourToMove;
	//
	//					rooks[ColourToMoveIndex].MovePiece(castlingRookFromIndex, castlingRookToIndex);
	//					ZobristKey ^= Zobrist.piecesArray[Piece.Rook, ColourToMoveIndex, castlingRookFromIndex];
	//					ZobristKey ^= Zobrist.piecesArray[Piece.Rook, ColourToMoveIndex, castlingRookToIndex];
	//					break;
	//				}
	//			}
	//
	//			// Update the board representation:
	//			Square[moveTo] = pieceOnTargetSquare;
	//			Square[moveFrom] = 0;
	//
	//			// Pawn has moved two forwards, mark file with en-passant flag
	//			if (moveFlag == Move.Flag.PawnTwoForward) {
	//				int file = BoardRepresentation.FileIndex(moveFrom) + 1;
	//				currentGameState |= (ushort)(file << 4);
	//				ZobristKey ^= Zobrist.enPassantFile[file];
	//			}
	//
	//			// Piece moving to/from rook square removes castling right for that side
	//			if (originalCastleState != 0) {
	//				if (moveTo == BoardRepresentation.h1 || moveFrom == BoardRepresentation.h1) {
	//					newCastleState &= whiteCastleKingsideMask;
	//				}
	//				else if (moveTo == BoardRepresentation.a1 || moveFrom == BoardRepresentation.a1) {
	//					newCastleState &= whiteCastleQueensideMask;
	//				}
	//				if (moveTo == BoardRepresentation.h8 || moveFrom == BoardRepresentation.h8) {
	//					newCastleState &= blackCastleKingsideMask;
	//				}
	//				else if (moveTo == BoardRepresentation.a8 || moveFrom == BoardRepresentation.a8) {
	//					newCastleState &= blackCastleQueensideMask;
	//				}
	//			}
	//
	//			// Update zobrist key with new piece position and side to move
	//			ZobristKey ^= Zobrist.sideToMove;
	//			ZobristKey ^= Zobrist.piecesArray[movePieceType, ColourToMoveIndex, moveFrom];
	//			ZobristKey ^= Zobrist.piecesArray[Piece.PieceType(pieceOnTargetSquare), ColourToMoveIndex, moveTo];
	//
	//			if (oldEnPassantFile != 0)
	//				ZobristKey ^= Zobrist.enPassantFile[oldEnPassantFile];
	//
	//			if (newCastleState != originalCastleState) {
	//				ZobristKey ^= Zobrist.castlingRights[originalCastleState]; // remove old castling rights state
	//				ZobristKey ^= Zobrist.castlingRights[newCastleState]; // add new castling rights state
	//			}
	//			currentGameState |= newCastleState;
	//			currentGameState |= (uint)fiftyMoveCounter << 14;
	//			gameStateHistory.Push(currentGameState);
	//
	//			// Change side to move
	//			WhiteToMove = !WhiteToMove;
	//			ColourToMove = (WhiteToMove) ? Piece.White : Piece.Black;
	//			OpponentColour = (WhiteToMove) ? Piece.Black : Piece.White;
	//			ColourToMoveIndex = 1 - ColourToMoveIndex;
	//			plyCount++;
	//			fiftyMoveCounter++;
	//
	//			if (!inSearch) {
	//				if (movePieceType == Piece.Pawn || capturedPieceType != Piece.None) {
	//					RepetitionPositionHistory.Clear();
	//					fiftyMoveCounter = 0;
	//				}
	//				else {
	//					RepetitionPositionHistory.Push(ZobristKey);
	//				}
	//			}
	//
	//		}
	//
	//		// Undo a move previously made on the board
	//		public void UnmakeMove(Move move, bool inSearch = false) {
	//
	//			//int opponentColour = ColourToMove;
	//			int opponentColourIndex = ColourToMoveIndex;
	//			bool undoingWhiteMove = OpponentColour == Piece.White;
	//			ColourToMove = OpponentColour; // side who made the move we are undoing
	//			OpponentColour = (undoingWhiteMove) ? Piece.Black : Piece.White;
	//			ColourToMoveIndex = 1 - ColourToMoveIndex;
	//			WhiteToMove = !WhiteToMove;
	//
	//			uint originalCastleState = currentGameState & 0b1111;
	//
	//			int capturedPieceType = ((int)currentGameState >> 8) & 63;
	//			int capturedPiece = (capturedPieceType == 0) ? 0 : capturedPieceType | OpponentColour;
	//
	//			int movedFrom = move.StartSquare;
	//			int movedTo = move.TargetSquare;
	//			int moveFlags = move.MoveFlag;
	//			bool isEnPassant = moveFlags == Move.Flag.EnPassantCapture;
	//			bool isPromotion = move.IsPromotion;
	//
	//			int toSquarePieceType = Piece.PieceType(Square[movedTo]);
	//			int movedPieceType = (isPromotion) ? Piece.Pawn : toSquarePieceType;
	//
	//			// Update zobrist key with new piece position and side to move
	//			ZobristKey ^= Zobrist.sideToMove;
	//			ZobristKey ^= Zobrist.piecesArray[movedPieceType, ColourToMoveIndex, movedFrom]; // add piece back to square it moved from
	//			ZobristKey ^= Zobrist.piecesArray[toSquarePieceType, ColourToMoveIndex, movedTo]; // remove piece from square it moved to
	//
	//			uint oldEnPassantFile = (currentGameState >> 4) & 15;
	//			if (oldEnPassantFile != 0)
	//				ZobristKey ^= Zobrist.enPassantFile[oldEnPassantFile];
	//
	//			// ignore ep captures, handled later
	//			if (capturedPieceType != 0 && !isEnPassant) {
	//				ZobristKey ^= Zobrist.piecesArray[capturedPieceType, opponentColourIndex, movedTo];
	//				GetPieceList(capturedPieceType, opponentColourIndex).AddPieceAtSquare(movedTo);
	//			}
	//
	//			// Update king index
	//			if (movedPieceType == Piece.King) {
	//				KingSquare[ColourToMoveIndex] = movedFrom;
	//			}
	//			else if (!isPromotion) {
	//				GetPieceList(movedPieceType, ColourToMoveIndex).MovePiece(movedTo, movedFrom);
	//			}
	//
	//			// put back moved piece
	//			Square[movedFrom] = movedPieceType | ColourToMove; // note that if move was a pawn promotion, this will put the promoted piece back instead of the pawn. Handled in special move switch
	//			Square[movedTo] = capturedPiece; // will be 0 if no piece was captured
	//
	//			if (isPromotion) {
	//				pawns[ColourToMoveIndex].AddPieceAtSquare(movedFrom);
	//				switch (moveFlags) {
	//				case Move.Flag.PromoteToQueen:
	//					queens[ColourToMoveIndex].RemovePieceAtSquare(movedTo);
	//					break;
	//				case Move.Flag.PromoteToKnight:
	//					knights[ColourToMoveIndex].RemovePieceAtSquare(movedTo);
	//					break;
	//				case Move.Flag.PromoteToRook:
	//					rooks[ColourToMoveIndex].RemovePieceAtSquare(movedTo);
	//					break;
	//				case Move.Flag.PromoteToBishop:
	//					bishops[ColourToMoveIndex].RemovePieceAtSquare(movedTo);
	//					break;
	//				}
	//			}
	//			else if (isEnPassant) { // ep cature: put captured pawn back on right square
	//				int epIndex = movedTo + ((ColourToMove == Piece.White) ? -8 : 8);
	//				Square[movedTo] = 0;
	//				Square[epIndex] = (int)capturedPiece;
	//				pawns[opponentColourIndex].AddPieceAtSquare(epIndex);
	//				ZobristKey ^= Zobrist.piecesArray[Piece.Pawn, opponentColourIndex, epIndex];
	//			}
	//			else if (moveFlags == Move.Flag.Castling) { // castles: move rook back to starting square
	//
	//				bool kingside = movedTo == 6 || movedTo == 62;
	//				int castlingRookFromIndex = (kingside) ? movedTo + 1 : movedTo - 2;
	//				int castlingRookToIndex = (kingside) ? movedTo - 1 : movedTo + 1;
	//
	//				Square[castlingRookToIndex] = 0;
	//				Square[castlingRookFromIndex] = Piece.Rook | ColourToMove;
	//
	//				rooks[ColourToMoveIndex].MovePiece(castlingRookToIndex, castlingRookFromIndex);
	//				ZobristKey ^= Zobrist.piecesArray[Piece.Rook, ColourToMoveIndex, castlingRookFromIndex];
	//				ZobristKey ^= Zobrist.piecesArray[Piece.Rook, ColourToMoveIndex, castlingRookToIndex];
	//
	//			}
	//
	//			gameStateHistory.Pop(); // removes current state from history
	//			currentGameState = gameStateHistory.Peek(); // sets current state to previous state in history
	//
	//			fiftyMoveCounter = (int)(currentGameState & 4294950912) >> 14;
	//			int newEnPassantFile = (int)(currentGameState >> 4) & 15;
	//			if (newEnPassantFile != 0)
	//				ZobristKey ^= Zobrist.enPassantFile[newEnPassantFile];
	//
	//			uint newCastleState = currentGameState & 0b1111;
	//			if (newCastleState != originalCastleState) {
	//				ZobristKey ^= Zobrist.castlingRights[originalCastleState]; // remove old castling rights state
	//				ZobristKey ^= Zobrist.castlingRights[newCastleState]; // add new castling rights state
	//			}
	//
	//			plyCount--;
	//
	//			if (!inSearch && RepetitionPositionHistory.Count > 0) {
	//				RepetitionPositionHistory.Pop();
	//			}
	//
	//		}
	//
	//		// Load the starting position
	//		public void LoadStartPosition() {
	//			LoadPosition(FenUtility.startFen);
	//		}
	//
	//		// Load custom position from fen string
	//		public void LoadPosition(string fen) {
	//			Initialize();
	//			var loadedPosition = FenUtility.PositionFromFen(fen);
	//
	//			// Load pieces into board array and piece lists
	//			for (int squareIndex = 0; squareIndex < 64; squareIndex++) {
	//				int piece = loadedPosition.squares[squareIndex];
	//				Square[squareIndex] = piece;
	//
	//				if (piece != Piece.None) {
	//					int pieceType = Piece.PieceType(piece);
	//					int pieceColourIndex = (Piece.IsColour(piece, Piece.White)) ? WhiteIndex : BlackIndex;
	//					if (Piece.IsSlidingPiece(piece)) {
	//						if (pieceType == Piece.Queen) {
	//							queens[pieceColourIndex].AddPieceAtSquare(squareIndex);
	//						}
	//						else if (pieceType == Piece.Rook) {
	//							rooks[pieceColourIndex].AddPieceAtSquare(squareIndex);
	//						}
	//						else if (pieceType == Piece.Bishop) {
	//							bishops[pieceColourIndex].AddPieceAtSquare(squareIndex);
	//						}
	//					}
	//					else if (pieceType == Piece.Knight) {
	//						knights[pieceColourIndex].AddPieceAtSquare(squareIndex);
	//					}
	//					else if (pieceType == Piece.Pawn) {
	//						pawns[pieceColourIndex].AddPieceAtSquare(squareIndex);
	//					}
	//					else if (pieceType == Piece.King) {
	//						KingSquare[pieceColourIndex] = squareIndex;
	//					}
	//				}
	//			}
	//
	//			// Side to move
	//			WhiteToMove = loadedPosition.whiteToMove;
	//			ColourToMove = (WhiteToMove) ? Piece.White : Piece.Black;
	//			OpponentColour = (WhiteToMove) ? Piece.Black : Piece.White;
	//			ColourToMoveIndex = (WhiteToMove) ? 0 : 1;
	//
	//			// Create gamestate
	//			int whiteCastle = ((loadedPosition.whiteCastleKingside) ? 1 << 0 : 0) | ((loadedPosition.whiteCastleQueenside) ? 1 << 1 : 0);
	//			int blackCastle = ((loadedPosition.blackCastleKingside) ? 1 << 2 : 0) | ((loadedPosition.blackCastleQueenside) ? 1 << 3 : 0);
	//			int epState = loadedPosition.epFile << 4;
	//			ushort initialGameState = (ushort)(whiteCastle | blackCastle | epState);
	//			gameStateHistory.Push(initialGameState);
	//			currentGameState = initialGameState;
	//			plyCount = loadedPosition.plyCount;
	//
	//			// Initialize zobrist key
	//			ZobristKey = Zobrist.CalculateZobristKey(this);
	//		}
	//
	//		void Initialize() {
	//			Square = new int[64];
	//			KingSquare = new int[2];
	//
	//			gameStateHistory = new Stack<uint>();
	//			ZobristKey = 0;
	//			RepetitionPositionHistory = new Stack<ulong>();
	//			plyCount = 0;
	//			fiftyMoveCounter = 0;
	//
	//			knights = new PieceList[]{ new PieceList(10), new PieceList(10) };
	//			pawns = new PieceList[]{ new PieceList(8), new PieceList(8) };
	//			rooks = new PieceList[]{ new PieceList(10), new PieceList(10) };
	//			bishops = new PieceList[]{ new PieceList(10), new PieceList(10) };
	//			queens = new PieceList[]{ new PieceList(9), new PieceList(9) };
	//			PieceList emptyList = new PieceList(0);
	//			allPieceLists = new PieceList[]{
	//				emptyList,
	//				emptyList,
	//				pawns[WhiteIndex],
	//				knights[WhiteIndex],
	//				emptyList,
	//				bishops[WhiteIndex],
	//				rooks[WhiteIndex],
	//				queens[WhiteIndex],
	//				emptyList,
	//				emptyList,
	//				pawns[BlackIndex],
	//				knights[BlackIndex],
	//				emptyList,
	//				bishops[BlackIndex],
	//				rooks[BlackIndex],
	//				queens[BlackIndex],
	//			};
	//		}
	//	};
	//
	//	// Board Representation
	//
	//	static std::string fileNames = "abcdefgh";
	//	static std::string rankNames = "12345678";
	//
	//	class BoardRepresentation {
	//	public:
	//		//const std::string fileNames = "abcdefgh";
	//		//const std::string rankNames = "12345678";
	//
	//		const int a1 = 0;
	//		const int b1 = 1;
	//		const int c1 = 2;
	//		const int d1 = 3;
	//		const int e1 = 4;
	//		const int f1 = 5;
	//		const int g1 = 6;
	//		const int h1 = 7;
	//
	//		const int a8 = 56;
	//		const int b8 = 57;
	//		const int c8 = 58;
	//		const int d8 = 59;
	//		const int e8 = 60;
	//		const int f8 = 61;
	//		const int g8 = 62;
	//		const int h8 = 63;
	//
	//		// Rank (0 to 7) of square 
	//		static int RankIndex(int squareIndex) {
	//			return squareIndex >> 3;
	//		}
	//
	//		// File (0 to 7) of square 
	//		static int FileIndex(int squareIndex) {
	//			return squareIndex & 0b000111;
	//		}
	//
	//		static int IndexFromCoord(int fileIndex, int rankIndex) {
	//			return rankIndex * 8 + fileIndex;
	//		}
	//
	//		static int IndexFromCoord(Coord coord) {
	//			return IndexFromCoord(coord.fileIndex, coord.rankIndex);
	//		}
	//
	//		static Coord CoordFromIndex(int squareIndex) {
	//			return Coord(FileIndex(squareIndex), RankIndex(squareIndex));
	//		}
	//
	//		static bool LightSquare(int fileIndex, int rankIndex) {
	//			return (fileIndex + rankIndex) % 2 != 0;
	//		}
	//
	//		static std::string SquareNameFromCoordinate(int fileIndex, int rankIndex) {
	//			return fileNames[fileIndex] + "" + (rankIndex + 1);
	//		}
	//
	//		static std::string SquareNameFromIndex(int squareIndex) {
	//			return SquareNameFromCoordinate(CoordFromIndex(squareIndex));
	//		}
	//
	//		static std::string SquareNameFromCoordinate(Coord coord) {
	//			return SquareNameFromCoordinate(coord.fileIndex, coord.rankIndex);
	//		}
	//	};
	//
	//	// Coord
	//	struct Coord {
	//
	//		Coord(int fileIndex, int rankIndex)
	//		{
	//			fileIndex = fileIndex;
	//			rankIndex = rankIndex;
	//		}
	//
	//		int fileIndex;
	//		int rankIndex;
	//
	//		bool IsLightSquare() {
	//			return (fileIndex + rankIndex) % 2 != 0;
	//		}
	//
	//		int CompareTo(Coord other) {
	//			return (fileIndex == other.fileIndex && rankIndex == other.rankIndex) ? 0 : 1;
	//		}
	//	};
	//
	//	//Fen Utility
	//	class FenUtility {
	//
	//		const std::map<char, int> pieceTypeFromSymbol = {
	//			{ 'k', Piece.King },{ 'p', Piece.Pawn },{ 'n', Piece.Knight }, {'b', Piece.Bishop}, {'r', Piece.Rook}, { 'q', Piece.Queen }
	//		};
	//
	//		const std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	//
	//		class LoadedPositionInfo {
	//		public:
	//			int* squares;
	//			bool whiteCastleKingside;
	//			bool whiteCastleQueenside;
	//			bool blackCastleKingside;
	//			bool blackCastleQueenside;
	//			int epFile;
	//			bool whiteToMove;
	//			int plyCount;
	//
	//			LoadedPositionInfo() {
	//				squares = new int[64];
	//			}
	//		};
	//
	//		// Load position from fen string
	//		LoadedPositionInfo PositionFromFen(std::string fen) {
	//
	//			LoadedPositionInfo loadedPositionInfo = LoadedPositionInfo();
	//			std::vector<std::string> sections;
	//
	//			while (true)
	//			{
	//				if (fen.find(' ') != std::string::npos)
	//				{
	//					sections.push_back(fen.substr(0, fen.find(' ')));
	//					fen = fen.substr(fen.find(' '));
	//				}
	//				else
	//				{
	//					sections.push_back(fen);
	//					break;
	//				}
	//			}
	//
	//
	//			int file = 0;
	//			int rank = 7;
	//
	//			for (char symbol : sections[0]) {
	//				if (symbol == '/') {
	//					file = 0;
	//					rank--;
	//				}
	//				else {
	//					if (isdigit(symbol)) {
	//						file += symbol - '0';
	//					}
	//					else {
	//						int pieceColour = (isupper(symbol)) ? Piece.White : Piece.Black;
	//						int pieceType = pieceTypeFromSymbol[tolower(symbol)];
	//						loadedPositionInfo.squares[rank * 8 + file] = pieceType | pieceColour;
	//						file++;
	//					}
	//				}
	//			}
	//
	//			loadedPositionInfo.whiteToMove = (sections[1] == "w");
	//
	//			std::string castlingRights = (sections.size() > 2) ? sections[2] : "KQkq";
	//			loadedPositionInfo.whiteCastleKingside = castlingRights.find("K") != std::string::npos;
	//			loadedPositionInfo.whiteCastleQueenside = castlingRights.find("Q") != std::string::npos;
	//			loadedPositionInfo.blackCastleKingside = castlingRights.find("k") != std::string::npos;
	//			loadedPositionInfo.blackCastleQueenside = castlingRights.find("q") != std::string::npos;
	//
	//			if (sections.size() > 3) {
	//				std::string enPassantFileName = std::string(1, sections[3][0]);
	//				if (fileNames.find(enPassantFileName) != std::string::npos) {
	//					loadedPositionInfo.epFile = fileNames.find(enPassantFileName) + 1;
	//				}
	//			}
	//
	//			// Half-move clock
	//			if (sections.size() > 4) {
	//				loadedPositionInfo.plyCount = std::stoi(sections[4]);
	//			}
	//			return loadedPositionInfo;
	//		}
	//
	//		// Get the fen string of the current position
	//		static std::string CurrentFen(Board board) {
	//			std::string fen = "";
	//			for (int rank = 7; rank >= 0; rank--) {
	//				int numEmptyFiles = 0;
	//				for (int file = 0; file < 8; file++) {
	//					int i = rank * 8 + file;
	//					int piece = board.Square[i];
	//					if (piece != 0) {
	//						if (numEmptyFiles != 0) {
	//							fen += numEmptyFiles;
	//							numEmptyFiles = 0;
	//						}
	//						bool isBlack = Piece.IsColour(piece, Piece.Black);
	//						int pieceType = Piece.PieceType(piece);
	//						char pieceChar = ' ';
	//						switch (pieceType) {
	//						case Piece.Rook:
	//							pieceChar = 'R';
	//							break;
	//						case Piece.Knight:
	//							pieceChar = 'N';
	//							break;
	//						case Piece.Bishop:
	//							pieceChar = 'B';
	//							break;
	//						case Piece.Queen:
	//							pieceChar = 'Q';
	//							break;
	//						case Piece.King:
	//							pieceChar = 'K';
	//							break;
	//						case Piece.Pawn:
	//							pieceChar = 'P';
	//							break;
	//						}
	//						fen += (isBlack) ? tolower(pieceChar) : pieceChar;
	//					}
	//					else {
	//						numEmptyFiles++;
	//					}
	//
	//				}
	//				if (numEmptyFiles != 0) {
	//					fen += numEmptyFiles;
	//				}
	//				if (rank != 0) {
	//					fen += '/';
	//				}
	//			}
	//
	//			// Side to move
	//			fen += ' ';
	//			fen += (board.WhiteToMove) ? 'w' : 'b';
	//
	//			// Castling
	//			bool whiteKingside = (board.currentGameState & 1) == 1;
	//			bool whiteQueenside = (board.currentGameState >> 1 & 1) == 1;
	//			bool blackKingside = (board.currentGameState >> 2 & 1) == 1;
	//			bool blackQueenside = (board.currentGameState >> 3 & 1) == 1;
	//			fen += ' ';
	//			fen += (whiteKingside) ? "K" : "";
	//			fen += (whiteQueenside) ? "Q" : "";
	//			fen += (blackKingside) ? "k" : "";
	//			fen += (blackQueenside) ? "q" : "";
	//			fen += ((board.currentGameState & 15) == 0) ? "-" : "";
	//
	//			// En-passant
	//			fen += ' ';
	//			int epFile = (int)(board.currentGameState >> 4) & 15;
	//			if (epFile == 0) {
	//				fen += '-';
	//			}
	//			else {
	//				std::string fileName = std::string(1, fileNames[epFile - 1]);
	//				int epRank = (board.WhiteToMove) ? 6 : 3;
	//				fen += fileName + std::to_string(epRank);
	//			}
	//
	//			// 50 move counter
	//			fen += ' ';
	//			fen += board.fiftyMoveCounter;
	//
	//			// Full-move count (should be one at start, and increase after each move by black)
	//			fen += ' ';
	//			fen += (board.plyCount / 2) + 1;
	//
	//			return fen;
	//		}
	//	};
	//
	//	// Game Manager
	//	class GameManager {
	//	public:
	//		enum Result { Playing, WhiteIsMated, BlackIsMated, Stalemate, Repetition, FiftyMoveRule, InsufficientMaterial }
	//
	//		event System.Action onPositionLoaded;
	//		event System.Action<Move> onMoveMade;
	//
	//		public enum PlayerType { Human, AI }
	//
	//		bool loadCustomPosition;
	//		string customPosition = "1rbq1r1k/2pp2pp/p1n3p1/2b1p3/R3P3/1BP2N2/1P3PPP/1NBQ1RK1 w - - 0 1";
	//
	//		PlayerType whitePlayerType;
	//		PlayerType blackPlayerType;
	//		AISettings aiSettings;
	//		Color[] colors;
	//
	//		bool useClocks;
	//		Clock whiteClock;
	//		Clock blackClock;
	//		TMPro.TMP_Text aiDiagnosticsUI;
	//		TMPro.TMP_Text resultUI;
	//
	//		Result gameResult;
	//
	//		Player whitePlayer;
	//		Player blackPlayer;
	//		Player playerToMove;
	//		List<Move> gameMoves;
	//		BoardUI boardUI;
	//
	//		unsigned long zobristDebug;
	//		Board board{ get; private set; }
	//		Board searchBoard; // Duplicate version of board used for ai search
	//
	//		void Start() {
	//			//Application.targetFrameRate = 60;
	//
	//			if (useClocks) {
	//				whiteClock.isTurnToMove = false;
	//				blackClock.isTurnToMove = false;
	//			}
	//
	//			boardUI = FindObjectOfType<BoardUI>();
	//			gameMoves = new List<Move>();
	//			board = new Board();
	//			searchBoard = new Board();
	//			aiSettings.diagnostics = new Search.SearchDiagnostics();
	//
	//			NewGame(whitePlayerType, blackPlayerType);
	//
	//		}
	//
	//		void Update() {
	//			zobristDebug = board.ZobristKey;
	//
	//			if (gameResult == Result.Playing) {
	//				LogAIDiagnostics();
	//
	//				playerToMove.Update();
	//
	//				if (useClocks) {
	//					whiteClock.isTurnToMove = board.WhiteToMove;
	//					blackClock.isTurnToMove = !board.WhiteToMove;
	//				}
	//			}
	//
	//			if (Input.GetKeyDown(KeyCode.E)) {
	//				ExportGame();
	//			}
	//
	//		}
	//
	//		void OnMoveChosen(Move move) {
	//			bool animateMove = playerToMove is AIPlayer;
	//			board.MakeMove(move);
	//			searchBoard.MakeMove(move);
	//
	//			gameMoves.Add(move);
	//			onMoveMade ? .Invoke(move);
	//			boardUI.OnMoveMade(board, move, animateMove);
	//
	//			NotifyPlayerToMove();
	//		}
	//
	//		public void NewGame(bool humanPlaysWhite) {
	//			boardUI.SetPerspective(humanPlaysWhite);
	//			NewGame((humanPlaysWhite) ? PlayerType.Human : PlayerType.AI, (humanPlaysWhite) ? PlayerType.AI : PlayerType.Human);
	//		}
	//
	//		public void NewComputerVersusComputerGame() {
	//			boardUI.SetPerspective(true);
	//			NewGame(PlayerType.AI, PlayerType.AI);
	//		}
	//
	//		void NewGame(PlayerType whitePlayerType, PlayerType blackPlayerType) {
	//			gameMoves.Clear();
	//			if (loadCustomPosition) {
	//				board.LoadPosition(customPosition);
	//				searchBoard.LoadPosition(customPosition);
	//			}
	//			else {
	//				board.LoadStartPosition();
	//				searchBoard.LoadStartPosition();
	//			}
	//			onPositionLoaded ? .Invoke();
	//			boardUI.UpdatePosition(board);
	//			boardUI.ResetSquareColours();
	//
	//			CreatePlayer(ref whitePlayer, whitePlayerType);
	//			CreatePlayer(ref blackPlayer, blackPlayerType);
	//
	//			gameResult = Result.Playing;
	//			PrintGameResult(gameResult);
	//
	//			NotifyPlayerToMove();
	//
	//		}
	//
	//		void LogAIDiagnostics() {
	//			string text = "";
	//			var d = aiSettings.diagnostics;
	//			//text += "AI Diagnostics";
	//			text += $"<color=#{ColorUtility.ToHtmlStringRGB(colors[3])}>Version 1.0\n";
	//			text += $"<color=#{ColorUtility.ToHtmlStringRGB(colors[0])}>Depth Searched: {d.lastCompletedDepth}";
	//			//text += $"\nPositions evaluated: {d.numPositionsEvaluated}";
	//
	//			string evalString = "";
	//			if (d.isBook) {
	//				evalString = "Book";
	//			}
	//			else {
	//				float displayEval = d.eval / 100f;
	//				if (playerToMove is AIPlayer && !board.WhiteToMove) {
	//					displayEval = -displayEval;
	//				}
	//				evalString = ($"{displayEval:00.00}").Replace(",", ".");
	//				if (Search.IsMateScore(d.eval)) {
	//					evalString = $"mate in {Search.NumPlyToMateFromScore(d.eval)} ply";
	//				}
	//			}
	//			text += $"\n<color=#{ColorUtility.ToHtmlStringRGB(colors[1])}>Eval: {evalString}";
	//			text += $"\n<color=#{ColorUtility.ToHtmlStringRGB(colors[2])}>Move: {d.moveVal}";
	//
	//			aiDiagnosticsUI.text = text;
	//		}
	//
	//		public void ExportGame() {
	//			string pgn = PGNCreator.CreatePGN(gameMoves.ToArray());
	//			string baseUrl = "https://www.lichess.org/paste?pgn=";
	//			string escapedPGN = UnityEngine.Networking.UnityWebRequest.EscapeURL(pgn);
	//			string url = baseUrl + escapedPGN;
	//
	//			Application.OpenURL(url);
	//			TextEditor t = new TextEditor();
	//			t.text = pgn;
	//			t.SelectAll();
	//			t.Copy();
	//		}
	//
	//		public void QuitGame() {
	//			Application.Quit();
	//		}
	//
	//		void NotifyPlayerToMove() {
	//			gameResult = GetGameState();
	//			PrintGameResult(gameResult);
	//
	//			if (gameResult == Result.Playing) {
	//				playerToMove = (board.WhiteToMove) ? whitePlayer : blackPlayer;
	//				playerToMove.NotifyTurnToMove();
	//
	//			}
	//			else {
	//				Debug.Log("Game Over");
	//			}
	//		}
	//
	//		void PrintGameResult(Result result) {
	//			float subtitleSize = resultUI.fontSize * 0.75f;
	//			string subtitleSettings = $"<color=#787878> <size={subtitleSize}>";
	//
	//			if (result == Result.Playing) {
	//				resultUI.text = "";
	//			}
	//			else if (result == Result.WhiteIsMated || result == Result.BlackIsMated) {
	//				resultUI.text = "Checkmate!";
	//			}
	//			else if (result == Result.FiftyMoveRule) {
	//				resultUI.text = "Draw";
	//				resultUI.text += subtitleSettings + "\n(50 move rule)";
	//			}
	//			else if (result == Result.Repetition) {
	//				resultUI.text = "Draw";
	//				resultUI.text += subtitleSettings + "\n(3-fold repetition)";
	//			}
	//			else if (result == Result.Stalemate) {
	//				resultUI.text = "Draw";
	//				resultUI.text += subtitleSettings + "\n(Stalemate)";
	//			}
	//			else if (result == Result.InsufficientMaterial) {
	//				resultUI.text = "Draw";
	//				resultUI.text += subtitleSettings + "\n(Insufficient material)";
	//			}
	//		}
	//
	//		Result GetGameState() {
	//			MoveGenerator moveGenerator = new MoveGenerator();
	//			var moves = moveGenerator.GenerateMoves(board);
	//
	//			// Look for mate/stalemate
	//			if (moves.Count == 0) {
	//				if (moveGenerator.InCheck()) {
	//					return (board.WhiteToMove) ? Result.WhiteIsMated : Result.BlackIsMated;
	//				}
	//				return Result.Stalemate;
	//			}
	//
	//			// Fifty move rule
	//			if (board.fiftyMoveCounter >= 100) {
	//				return Result.FiftyMoveRule;
	//			}
	//
	//			// Threefold repetition
	//			int repCount = board.RepetitionPositionHistory.Count((x = > x == board.ZobristKey));
	//			if (repCount == 3) {
	//				return Result.Repetition;
	//			}
	//
	//			// Look for insufficient material (not all cases implemented yet)
	//			int numPawns = board.pawns[Board.WhiteIndex].Count + board.pawns[Board.BlackIndex].Count;
	//			int numRooks = board.rooks[Board.WhiteIndex].Count + board.rooks[Board.BlackIndex].Count;
	//			int numQueens = board.queens[Board.WhiteIndex].Count + board.queens[Board.BlackIndex].Count;
	//			int numKnights = board.knights[Board.WhiteIndex].Count + board.knights[Board.BlackIndex].Count;
	//			int numBishops = board.bishops[Board.WhiteIndex].Count + board.bishops[Board.BlackIndex].Count;
	//
	//			if (numPawns + numRooks + numQueens == 0) {
	//				if (numKnights == 1 || numBishops == 1) {
	//					return Result.InsufficientMaterial;
	//				}
	//			}
	//
	//			return Result.Playing;
	//		}
	//
	//		void CreatePlayer(ref Player player, PlayerType playerType) {
	//			if (player != null) {
	//				player.onMoveChosen -= OnMoveChosen;
	//			}
	//
	//			if (playerType == PlayerType.Human) {
	//				player = new HumanPlayer(board);
	//			}
	//			else {
	//				player = new AIPlayer(searchBoard, aiSettings);
	//			}
	//			player.onMoveChosen += OnMoveChosen;
	//		}
	//	};
	//
	//	// Human Player
	//	class HumanPlayer : Player {
	//	public:
	//		enum InputState {
	//			None,
	//			PieceSelected,
	//			DraggingPiece
	//		}
	//
	//		InputState currentState;
	//
	//		BoardUI boardUI;
	//		Camera cam;
	//		Coord selectedPieceSquare;
	//		Board board;
	//		HumanPlayer(Board board) {
	//			boardUI = GameObject.FindObjectOfType<BoardUI>();
	//			cam = Camera.main;
	//			this.board = board;
	//		}
	//
	//		override void NotifyTurnToMove() {
	//
	//		}
	//
	//		override void Update() {
	//			HandleInput();
	//		}
	//
	//		void HandleInput() {
	//			Vector2 mousePos = cam.ScreenToWorldPoint(Input.mousePosition);
	//
	//			if (currentState == InputState.None) {
	//				HandlePieceSelection(mousePos);
	//			}
	//			else if (currentState == InputState.DraggingPiece) {
	//				HandleDragMovement(mousePos);
	//			}
	//			else if (currentState == InputState.PieceSelected) {
	//				HandlePointAndClickMovement(mousePos);
	//			}
	//
	//			if (Input.GetMouseButtonDown(1)) {
	//				CancelPieceSelection();
	//			}
	//		}
	//
	//		void HandlePointAndClickMovement(Vector2 mousePos) {
	//			if (Input.GetMouseButton(0)) {
	//				HandlePiecePlacement(mousePos);
	//			}
	//		}
	//
	//		void HandleDragMovement(Vector2 mousePos) {
	//			boardUI.DragPiece(selectedPieceSquare, mousePos);
	//			// If mouse is released, then try place the piece
	//			if (Input.GetMouseButtonUp(0)) {
	//				HandlePiecePlacement(mousePos);
	//			}
	//		}
	//
	//		void HandlePiecePlacement(Vector2 mousePos) {
	//			Coord targetSquare;
	//			if (boardUI.TryGetSquareUnderMouse(mousePos, out targetSquare)) {
	//				if (targetSquare.Equals(selectedPieceSquare)) {
	//					boardUI.ResetPiecePosition(selectedPieceSquare);
	//					if (currentState == InputState.DraggingPiece) {
	//						currentState = InputState.PieceSelected;
	//					}
	//					else {
	//						currentState = InputState.None;
	//						boardUI.DeselectSquare(selectedPieceSquare);
	//					}
	//				}
	//				else {
	//					int targetIndex = BoardRepresentation.IndexFromCoord(targetSquare.fileIndex, targetSquare.rankIndex);
	//					if (Piece.IsColour(board.Square[targetIndex], board.ColourToMove) && board.Square[targetIndex] != 0) {
	//						CancelPieceSelection();
	//						HandlePieceSelection(mousePos);
	//					}
	//					else {
	//						TryMakeMove(selectedPieceSquare, targetSquare);
	//					}
	//				}
	//			}
	//			else {
	//				CancelPieceSelection();
	//			}
	//
	//		}
	//
	//		void CancelPieceSelection() {
	//			if (currentState != InputState.None) {
	//				currentState = InputState.None;
	//				boardUI.DeselectSquare(selectedPieceSquare);
	//				boardUI.ResetPiecePosition(selectedPieceSquare);
	//			}
	//		}
	//
	//		void TryMakeMove(Coord startSquare, Coord targetSquare) {
	//			int startIndex = BoardRepresentation.IndexFromCoord(startSquare);
	//			int targetIndex = BoardRepresentation.IndexFromCoord(targetSquare);
	//			bool moveIsLegal = false;
	//			Move chosenMove = new Move();
	//
	//			MoveGenerator moveGenerator = new MoveGenerator();
	//			bool wantsKnightPromotion = Input.GetKey(KeyCode.LeftAlt);
	//
	//			var legalMoves = moveGenerator.GenerateMoves(board);
	//			for (int i = 0; i < legalMoves.Count; i++) {
	//				var legalMove = legalMoves[i];
	//
	//				if (legalMove.StartSquare == startIndex && legalMove.TargetSquare == targetIndex) {
	//					if (legalMove.IsPromotion) {
	//						if (legalMove.MoveFlag == Move.Flag.PromoteToQueen && wantsKnightPromotion) {
	//							continue;
	//						}
	//						if (legalMove.MoveFlag != Move.Flag.PromoteToQueen && !wantsKnightPromotion) {
	//							continue;
	//						}
	//					}
	//					moveIsLegal = true;
	//					chosenMove = legalMove;
	//					//	Debug.Log (legalMove.PromotionPieceType);
	//					break;
	//				}
	//			}
	//
	//			if (moveIsLegal) {
	//				ChoseMove(chosenMove);
	//				currentState = InputState.None;
	//			}
	//			else {
	//				CancelPieceSelection();
	//			}
	//		}
	//
	//		void HandlePieceSelection(Vector2 mousePos) {
	//			if (Dymatic::Input::IsMouseButtonPressed(Dymatic::Mouse::ButtonLeft)) {
	//				if (boardUI.TryGetSquareUnderMouse(mousePos, out selectedPieceSquare)) {
	//					int index = BoardRepresentation.IndexFromCoord(selectedPieceSquare);
	//					// If square contains a piece, select that piece for dragging
	//					if (Piece.IsColour(board.Square[index], board.ColourToMove)) {
	//						boardUI.HighlightLegalMoves(board, selectedPieceSquare);
	//						boardUI.SelectSquare(selectedPieceSquare);
	//						currentState = InputState.DraggingPiece;
	//					}
	//				}
	//			}
	//		}
	//	};
	//	
	//	// Move
	//	struct Move {
	//
	//		//struct Flag {
	//		//	static int None = 0;
	//		//	static int EnPassantCapture = 1;
	//		//	static int Castling = 2;
	//		//	static int PromoteToQueen = 3;
	//		//	static int PromoteToKnight = 4;
	//		//	static int PromoteToRook = 5;
	//		//	static int PromoteToBishop = 6;
	//		//	static int PawnTwoForward = 7;
	//		//};
	//
	//		enum Flag
	//		{
	//			None = 0,
	//			EnPassantCapture = 1,
	//			Castling = 2,
	//			PromoteToQueen = 3,
	//			PromoteToKnight = 4,
	//			PromoteToRook = 5,
	//			PromoteToBishop = 6,
	//			PawnTwoForward = 7
	//		};
	//
	//		unsigned short moveValue;
	//
	//		const unsigned short startSquareMask = 0b0000000000111111;
	//		const unsigned short targetSquareMask = 0b0000111111000000;
	//		const unsigned short flagMask = 0b1111000000000000;
	//
	//		Move(unsigned short MoveValue) {
	//			moveValue = MoveValue;
	//		}
	//
	//		Move(int startSquare, int targetSquare) {
	//			moveValue = (unsigned short)(startSquare | targetSquare << 6);
	//		}
	//
	//		Move(int startSquare, int targetSquare, int flag) {
	//			moveValue = (unsigned short)(startSquare | targetSquare << 6 | flag << 12);
	//		}
	//
	//		int StartSquare() {
	//			return moveValue & startSquareMask;
	//		}
	//
	//		int TargetSquare() {
	//			return (moveValue & targetSquareMask) >> 6;
	//		}
	//
	//		bool IsPromotion() {
	//			int flag = MoveFlag();
	//			return flag == Flag::PromoteToQueen || flag == Flag::PromoteToRook || flag == Flag::PromoteToKnight || flag == Flag::PromoteToBishop;
	//		}
	//
	//		int MoveFlag() {
	//			return moveValue >> 12;
	//		}
	//
	//		int PromotionPieceType() {
	//			switch (MoveFlag()) {
	//			case Flag::PromoteToRook:
	//				return Piece.Rook;
	//			case Flag::PromoteToKnight:
	//				return Piece.Knight;
	//			case Flag::PromoteToBishop:
	//				return Piece.Bishop;
	//			case Flag::PromoteToQueen:
	//				return Piece.Queen;
	//			default:
	//				return Piece.None;
	//			}
	//		}
	//
	//		static Move InvalidMove() {
	//			return Move(0);
	//		}
	//
	//		static bool SameMove(Move a, Move b) {
	//			return a.moveValue == b.moveValue;
	//		}
	//
	//		unsigned short Value() {
	//			return moveValue;
	//		}
	//
	//		bool IsInvalid() {
	//			return moveValue == 0;
	//		}
	//
	//		std::string Name() {
	//			return BoardRepresentation.SquareNameFromIndex(StartSquare) + "-" + BoardRepresentation.SquareNameFromIndex(TargetSquare);
	//		}
	//	};
	//
	//	// Move Generator
	//	class MoveGenerator {
	//
	//		enum PromotionMode { All, QueenOnly, QueenAndKnight };
	//
	//		PromotionMode promotionsToGenerate = PromotionMode::All;
	//
	//		// ---- Instance variables ----
	//		std::vector<Move> moves;
	//		bool isWhiteToMove;
	//		int friendlyColour;
	//		int opponentColour;
	//		int friendlyKingSquare;
	//		int friendlyColourIndex;
	//		int opponentColourIndex;
	//
	//		bool inCheck;
	//		bool inDoubleCheck;
	//		bool pinsExistInPosition;
	//		unsigned long checkRayBitmask;
	//		unsigned long pinRayBitmask;
	//		unsigned long opponentKnightAttacks;
	//		unsigned long opponentAttackMapNoPawns;
	//		unsigned long opponentAttackMap;
	//		unsigned long opponentPawnAttackMap;
	//		unsigned long opponentSlidingAttackMap;
	//
	//		bool genQuiets;
	//		Board board;
	//
	//		// Generates list of legal moves in current position.
	//		// Quiet moves (non captures) can optionally be excluded. This is used in quiescence search.
	//		std::vector<Move> GenerateMoves(Board boardIn, bool includeQuietMoves = true) {
	//			board = boardIn;
	//			genQuiets = includeQuietMoves;
	//			Init();
	//
	//			CalculateAttackData();
	//			GenerateKingMoves();
	//
	//			// Only king moves are valid in a double check position, so can return early.
	//			if (inDoubleCheck) {
	//				return moves;
	//			}
	//
	//			GenerateSlidingMoves();
	//			GenerateKnightMoves();
	//			GeneratePawnMoves();
	//
	//			return moves;
	//		}
	//
	//		// Note, this will only return correct value after GenerateMoves() has been called in the current position
	//		bool InCheck() {
	//			return inCheck;
	//		}
	//
	//		void Init() {
	//			moves = std::vector<Move>(64);
	//			inCheck = false;
	//			inDoubleCheck = false;
	//			pinsExistInPosition = false;
	//			checkRayBitmask = 0;
	//			pinRayBitmask = 0;
	//
	//			isWhiteToMove = board.ColourToMove == Piece.White;
	//			friendlyColour = board.ColourToMove;
	//			opponentColour = board.OpponentColour;
	//			friendlyKingSquare = board.KingSquare[board.ColourToMoveIndex];
	//			friendlyColourIndex = (board.WhiteToMove) ? WhiteIndex : BlackIndex;
	//			opponentColourIndex = 1 - friendlyColourIndex;
	//		}
	//
	//		void GenerateKingMoves() {
	//			for (int i = 0; i < kingMoves[friendlyKingSquare].Size(); i++) {
	//				int targetSquare = kingMoves[friendlyKingSquare][i];
	//				int pieceOnTargetSquare = board.Square[targetSquare];
	//
	//				// Skip squares occupied by friendly pieces
	//				if (Piece.IsColour(pieceOnTargetSquare, friendlyColour)) {
	//					continue;
	//				}
	//
	//				bool isCapture = Piece.IsColour(pieceOnTargetSquare, opponentColour);
	//				if (!isCapture) {
	//					// King can't move to square marked as under enemy control, unless he is capturing that piece
	//					// Also skip if not generating quiet moves
	//					if (!genQuiets || SquareIsInCheckRay(targetSquare)) {
	//						continue;
	//					}
	//				}
	//
	//				// Safe for king to move to this square
	//				if (!SquareIsAttacked(targetSquare)) {
	//					moves.push_back(Move(friendlyKingSquare, targetSquare));
	//
	//					// Castling:
	//					if (!inCheck && !isCapture) {
	//						// Castle kingside
	//						if ((targetSquare == f1 || targetSquare == f8) && HasKingsideCastleRight) {
	//							int castleKingsideSquare = targetSquare + 1;
	//							if (board.Square[castleKingsideSquare] == Piece.None) {
	//								if (!SquareIsAttacked(castleKingsideSquare)) {
	//									moves.Add(new Move(friendlyKingSquare, castleKingsideSquare, Move.Flag.Castling));
	//								}
	//							}
	//						}
	//						// Castle queenside
	//						else if ((targetSquare == d1 || targetSquare == d8) && HasQueensideCastleRight) {
	//							int castleQueensideSquare = targetSquare - 1;
	//							if (board.Square[castleQueensideSquare] == Piece.None && board.Square[castleQueensideSquare - 1] == Piece.None) {
	//								if (!SquareIsAttacked(castleQueensideSquare)) {
	//									moves.Add(new Move(friendlyKingSquare, castleQueensideSquare, Move::Flag::Castling));
	//								}
	//							}
	//						}
	//					}
	//				}
	//			}
	//		}
	//
	//		void GenerateSlidingMoves() {
	//			PieceList rooks = board.rooks[friendlyColourIndex];
	//			for (int i = 0; i < rooks.Count; i++) {
	//				GenerateSlidingPieceMoves(rooks[i], 0, 4);
	//			}
	//
	//			PieceList bishops = board.bishops[friendlyColourIndex];
	//			for (int i = 0; i < bishops.Count; i++) {
	//				GenerateSlidingPieceMoves(bishops[i], 4, 8);
	//			}
	//
	//			PieceList queens = board.queens[friendlyColourIndex];
	//			for (int i = 0; i < queens.Count; i++) {
	//				GenerateSlidingPieceMoves(queens[i], 0, 8);
	//			}
	//
	//		}
	//
	//		void GenerateSlidingPieceMoves(int startSquare, int startDirIndex, int endDirIndex) {
	//			bool isPinned = IsPinned(startSquare);
	//
	//			// If this piece is pinned, and the king is in check, this piece cannot move
	//			if (inCheck && isPinned) {
	//				return;
	//			}
	//
	//			for (int directionIndex = startDirIndex; directionIndex < endDirIndex; directionIndex++) {
	//				int currentDirOffset = directionOffsets[directionIndex];
	//
	//				// If pinned, this piece can only move along the ray towards/away from the friendly king, so skip other directions
	//				if (isPinned && !IsMovingAlongRay(currentDirOffset, friendlyKingSquare, startSquare)) {
	//					continue;
	//				}
	//
	//				for (int n = 0; n < numSquaresToEdge[startSquare][directionIndex]; n++) {
	//					int targetSquare = startSquare + currentDirOffset * (n + 1);
	//					int targetSquarePiece = board.Square[targetSquare];
	//
	//					// Blocked by friendly piece, so stop looking in this direction
	//					if (Piece.IsColour(targetSquarePiece, friendlyColour)) {
	//						break;
	//					}
	//					bool isCapture = targetSquarePiece != Piece.None;
	//
	//					bool movePreventsCheck = SquareIsInCheckRay(targetSquare);
	//					if (movePreventsCheck || !inCheck) {
	//						if (genQuiets || isCapture) {
	//							moves.Add(new Move(startSquare, targetSquare));
	//						}
	//					}
	//					// If square not empty, can't move any further in this direction
	//					// Also, if this move blocked a check, further moves won't block the check
	//					if (isCapture || movePreventsCheck) {
	//						break;
	//					}
	//				}
	//			}
	//		}
	//
	//		void GenerateKnightMoves() {
	//			PieceList myKnights = board.knights[friendlyColourIndex];
	//
	//			for (int i = 0; i < myKnights.Count; i++) {
	//				int startSquare = myKnights[i];
	//
	//				// Knight cannot move if it is pinned
	//				if (IsPinned(startSquare)) {
	//					continue;
	//				}
	//
	//				for (int knightMoveIndex = 0; knightMoveIndex < knightMoves[startSquare].Length; knightMoveIndex++) {
	//					int targetSquare = knightMoves[startSquare][knightMoveIndex];
	//					int targetSquarePiece = board.Square[targetSquare];
	//					bool isCapture = Piece.IsColour(targetSquarePiece, opponentColour);
	//					if (genQuiets || isCapture) {
	//						// Skip if square contains friendly piece, or if in check and knight is not interposing/capturing checking piece
	//						if (Piece.IsColour(targetSquarePiece, friendlyColour) || (inCheck && !SquareIsInCheckRay(targetSquare))) {
	//							continue;
	//						}
	//						moves.Add(new Move(startSquare, targetSquare));
	//					}
	//				}
	//			}
	//		}
	//
	//		void GeneratePawnMoves() {
	//			PieceList myPawns = board.pawns[friendlyColourIndex];
	//			int pawnOffset = (friendlyColour == Piece.White) ? 8 : -8;
	//			int startRank = (board.WhiteToMove) ? 1 : 6;
	//			int finalRankBeforePromotion = (board.WhiteToMove) ? 6 : 1;
	//
	//			int enPassantFile = ((int)(board.currentGameState >> 4) & 15) - 1;
	//			int enPassantSquare = -1;
	//			if (enPassantFile != -1) {
	//				enPassantSquare = 8 * ((board.WhiteToMove) ? 5 : 2) + enPassantFile;
	//			}
	//
	//			for (int i = 0; i < myPawns.Count; i++) {
	//				int startSquare = myPawns[i];
	//				int rank = RankIndex(startSquare);
	//				bool oneStepFromPromotion = rank == finalRankBeforePromotion;
	//
	//				if (genQuiets) {
	//
	//					int squareOneForward = startSquare + pawnOffset;
	//
	//					// Square ahead of pawn is empty: forward moves
	//					if (board.Square[squareOneForward] == Piece.None) {
	//						// Pawn not pinned, or is moving along line of pin
	//						if (!IsPinned(startSquare) || IsMovingAlongRay(pawnOffset, startSquare, friendlyKingSquare)) {
	//							// Not in check, or pawn is interposing checking piece
	//							if (!inCheck || SquareIsInCheckRay(squareOneForward)) {
	//								if (oneStepFromPromotion) {
	//									MakePromotionMoves(startSquare, squareOneForward);
	//								}
	//								else {
	//									moves.Add(new Move(startSquare, squareOneForward));
	//								}
	//							}
	//
	//							// Is on starting square (so can move two forward if not blocked)
	//							if (rank == startRank) {
	//								int squareTwoForward = squareOneForward + pawnOffset;
	//								if (board.Square[squareTwoForward] == Piece.None) {
	//									// Not in check, or pawn is interposing checking piece
	//									if (!inCheck || SquareIsInCheckRay(squareTwoForward)) {
	//										moves.Add(new Move(startSquare, squareTwoForward, Move.Flag.PawnTwoForward));
	//									}
	//								}
	//							}
	//						}
	//					}
	//				}
	//
	//				// Pawn captures.
	//				for (int j = 0; j < 2; j++) {
	//					// Check if square exists diagonal to pawn
	//					if (numSquaresToEdge[startSquare][pawnAttackDirections[friendlyColourIndex][j]] > 0) {
	//						// move in direction friendly pawns attack to get square from which enemy pawn would attack
	//						int pawnCaptureDir = directionOffsets[pawnAttackDirections[friendlyColourIndex][j]];
	//						int targetSquare = startSquare + pawnCaptureDir;
	//						int targetPiece = board.Square[targetSquare];
	//
	//						// If piece is pinned, and the square it wants to move to is not on same line as the pin, then skip this direction
	//						if (IsPinned(startSquare) && !IsMovingAlongRay(pawnCaptureDir, friendlyKingSquare, startSquare)) {
	//							continue;
	//						}
	//
	//						// Regular capture
	//						if (Piece.IsColour(targetPiece, opponentColour)) {
	//							// If in check, and piece is not capturing/interposing the checking piece, then skip to next square
	//							if (inCheck && !SquareIsInCheckRay(targetSquare)) {
	//								continue;
	//							}
	//							if (oneStepFromPromotion) {
	//								MakePromotionMoves(startSquare, targetSquare);
	//							}
	//							else {
	//								moves.Add(new Move(startSquare, targetSquare));
	//							}
	//						}
	//
	//						// Capture en-passant
	//						if (targetSquare == enPassantSquare) {
	//							int epCapturedPawnSquare = targetSquare + ((board.WhiteToMove) ? -8 : 8);
	//							if (!InCheckAfterEnPassant(startSquare, targetSquare, epCapturedPawnSquare)) {
	//								moves.push_back(new Move(startSquare, targetSquare, Move::Flag::EnPassantCapture));
	//							}
	//						}
	//					}
	//				}
	//			}
	//		}
	//
	//		void MakePromotionMoves(int fromSquare, int toSquare) {
	//			moves.push_back(Move(fromSquare, toSquare, Move::Flag::PromoteToQueen));
	//			if (promotionsToGenerate == PromotionMode::All) {
	//				moves.push_back(Move(fromSquare, toSquare, Move::Flag::PromoteToKnight));
	//				moves.push_back(Move(fromSquare, toSquare, Move::Flag::PromoteToRook));
	//				moves.push_back(Move(fromSquare, toSquare, Move::Flag::PromoteToBishop));
	//			}
	//			else if (promotionsToGenerate == PromotionMode::QueenAndKnight) {
	//				moves.push_back(Move(fromSquare, toSquare, Move::Flag::PromoteToKnight));
	//			}
	//
	//		}
	//
	//		bool IsMovingAlongRay(int rayDir, int startSquare, int targetSquare) {
	//			int moveDir = directionLookup[targetSquare - startSquare + 63];
	//			return (rayDir == moveDir || -rayDir == moveDir);
	//		}
	//
	//		//bool IsMovingAlongRay (int directionOffset, int absRayOffset) {
	//		//return !((directionOffset == 1 || directionOffset == -1) && absRayOffset >= 7) && absRayOffset % directionOffset == 0;
	//		//}
	//
	//		bool IsPinned(int square) {
	//			return pinsExistInPosition && ((pinRayBitmask >> square) & 1) != 0;
	//		}
	//
	//		bool SquareIsInCheckRay(int square) {
	//			return inCheck && ((checkRayBitmask >> square) & 1) != 0;
	//		}
	//
	//		bool HasKingsideCastleRight() {
	//			int mask = (board.WhiteToMove) ? 1 : 4;
	//			return (board.currentGameState & mask) != 0;
	//		}
	//
	//		bool HasQueensideCastleRight() {
	//			int mask = (board.WhiteToMove) ? 2 : 8;
	//			return (board.currentGameState & mask) != 0;
	//		}
	//
	//		void GenSlidingAttackMap() {
	//			opponentSlidingAttackMap = 0;
	//
	//			PieceList enemyRooks = board.rooks[opponentColourIndex];
	//			for (int i = 0; i < enemyRooks.Count; i++) {
	//				UpdateSlidingAttackPiece(enemyRooks[i], 0, 4);
	//			}
	//
	//			PieceList enemyQueens = board.queens[opponentColourIndex];
	//			for (int i = 0; i < enemyQueens.Count; i++) {
	//				UpdateSlidingAttackPiece(enemyQueens[i], 0, 8);
	//			}
	//
	//			PieceList enemyBishops = board.bishops[opponentColourIndex];
	//			for (int i = 0; i < enemyBishops.Count; i++) {
	//				UpdateSlidingAttackPiece(enemyBishops[i], 4, 8);
	//			}
	//		}
	//
	//		void UpdateSlidingAttackPiece(int startSquare, int startDirIndex, int endDirIndex) {
	//
	//			for (int directionIndex = startDirIndex; directionIndex < endDirIndex; directionIndex++) {
	//				int currentDirOffset = directionOffsets[directionIndex];
	//				for (int n = 0; n < numSquaresToEdge[startSquare][directionIndex]; n++) {
	//					int targetSquare = startSquare + currentDirOffset * (n + 1);
	//					int targetSquarePiece = board.Square[targetSquare];
	//					opponentSlidingAttackMap |= 1ul << targetSquare;
	//					if (targetSquare != friendlyKingSquare) {
	//						if (targetSquarePiece != Piece.None) {
	//							break;
	//						}
	//					}
	//				}
	//			}
	//		}
	//
	//		void CalculateAttackData() {
	//			GenSlidingAttackMap();
	//			// Search squares in all directions around friendly king for checks/pins by enemy sliding pieces (queen, rook, bishop)
	//			int startDirIndex = 0;
	//			int endDirIndex = 8;
	//
	//			if (board.queens[opponentColourIndex].Count == 0) {
	//				startDirIndex = (board.rooks[opponentColourIndex].Count > 0) ? 0 : 4;
	//				endDirIndex = (board.bishops[opponentColourIndex].Count > 0) ? 8 : 4;
	//			}
	//
	//			for (int dir = startDirIndex; dir < endDirIndex; dir++) {
	//				bool isDiagonal = dir > 3;
	//
	//				int n = numSquaresToEdge[friendlyKingSquare][dir];
	//				int directionOffset = directionOffsets[dir];
	//				bool isFriendlyPieceAlongRay = false;
	//				unsigned long rayMask = 0;
	//
	//				for (int i = 0; i < n; i++) {
	//					int squareIndex = friendlyKingSquare + directionOffset * (i + 1);
	//					rayMask |= 1ul << squareIndex;
	//					int piece = board.Square[squareIndex];
	//
	//					// This square contains a piece
	//					if (piece != Piece.None) {
	//						if (Piece.IsColour(piece, friendlyColour)) {
	//							// First friendly piece we have come across in this direction, so it might be pinned
	//							if (!isFriendlyPieceAlongRay) {
	//								isFriendlyPieceAlongRay = true;
	//							}
	//							// This is the second friendly piece we've found in this direction, therefore pin is not possible
	//							else {
	//								break;
	//							}
	//						}
	//						// This square contains an enemy piece
	//						else {
	//							int pieceType = Piece.PieceType(piece);
	//
	//							// Check if piece is in bitmask of pieces able to move in current direction
	//							if (isDiagonal && Piece.IsBishopOrQueen(pieceType) || !isDiagonal && Piece.IsRookOrQueen(pieceType)) {
	//								// Friendly piece blocks the check, so this is a pin
	//								if (isFriendlyPieceAlongRay) {
	//									pinsExistInPosition = true;
	//									pinRayBitmask |= rayMask;
	//								}
	//								// No friendly piece blocking the attack, so this is a check
	//								else {
	//									checkRayBitmask |= rayMask;
	//									inDoubleCheck = inCheck; // if already in check, then this is double check
	//									inCheck = true;
	//								}
	//								break;
	//							}
	//							else {
	//								// This enemy piece is not able to move in the current direction, and so is blocking any checks/pins
	//								break;
	//							}
	//						}
	//					}
	//				}
	//				// Stop searching for pins if in double check, as the king is the only piece able to move in that case anyway
	//				if (inDoubleCheck) {
	//					break;
	//				}
	//
	//			}
	//
	//			// Knight attacks
	//			PieceList opponentKnights = board.knights[opponentColourIndex];
	//			opponentKnightAttacks = 0;
	//			bool isKnightCheck = false;
	//
	//			for (int knightIndex = 0; knightIndex < opponentKnights.Count; knightIndex++) {
	//				int startSquare = opponentKnights[knightIndex];
	//				opponentKnightAttacks |= knightAttackBitboards[startSquare];
	//
	//				if (!isKnightCheck && BitBoardUtility::ContainsSquare(opponentKnightAttacks, friendlyKingSquare)) {
	//					isKnightCheck = true;
	//					inDoubleCheck = inCheck; // if already in check, then this is double check
	//					inCheck = true;
	//					checkRayBitmask |= 1ul << startSquare;
	//				}
	//			}
	//
	//			// Pawn attacks
	//			PieceList opponentPawns = board.pawns[opponentColourIndex];
	//			opponentPawnAttackMap = 0;
	//			bool isPawnCheck = false;
	//
	//			for (int pawnIndex = 0; pawnIndex < opponentPawns.Count; pawnIndex++) {
	//				int pawnSquare = opponentPawns[pawnIndex];
	//				unsigned long pawnAttacks = pawnAttackBitboards[pawnSquare][opponentColourIndex];
	//				opponentPawnAttackMap |= pawnAttacks;
	//
	//				if (!isPawnCheck && BitBoardUtility::ContainsSquare(pawnAttacks, friendlyKingSquare)) {
	//					isPawnCheck = true;
	//					inDoubleCheck = inCheck; // if already in check, then this is double check
	//					inCheck = true;
	//					checkRayBitmask |= 1ul << pawnSquare;
	//				}
	//			}
	//
	//			int enemyKingSquare = board.KingSquare[opponentColourIndex];
	//
	//			opponentAttackMapNoPawns = opponentSlidingAttackMap | opponentKnightAttacks | kingAttackBitboards[enemyKingSquare];
	//			opponentAttackMap = opponentAttackMapNoPawns | opponentPawnAttackMap;
	//		}
	//
	//		bool SquareIsAttacked(int square) {
	//			return BitBoardUtility::ContainsSquare(opponentAttackMap, square);
	//		}
	//
	//		bool InCheckAfterEnPassant(int startSquare, int targetSquare, int epCapturedPawnSquare) {
	//			// Update board to reflect en-passant capture
	//			board.Square[targetSquare] = board.Square[startSquare];
	//			board.Square[startSquare] = Piece.None;
	//			board.Square[epCapturedPawnSquare] = Piece.None;
	//
	//			bool inCheckAfterEpCapture = false;
	//			if (SquareAttackedAfterEPCapture(epCapturedPawnSquare, startSquare)) {
	//				inCheckAfterEpCapture = true;
	//			}
	//
	//			// Undo change to board
	//			board.Square[targetSquare] = Piece.None;
	//			board.Square[startSquare] = Piece.Pawn | friendlyColour;
	//			board.Square[epCapturedPawnSquare] = Piece.Pawn | opponentColour;
	//			return inCheckAfterEpCapture;
	//		}
	//
	//		bool SquareAttackedAfterEPCapture(int epCaptureSquare, int capturingPawnStartSquare) {
	//			if (BitBoardUtility::ContainsSquare(opponentAttackMapNoPawns, friendlyKingSquare)) {
	//				return true;
	//			}
	//
	//			// Loop through the horizontal direction towards ep capture to see if any enemy piece now attacks king
	//			int dirIndex = (epCaptureSquare < friendlyKingSquare) ? 2 : 3;
	//			for (int i = 0; i < numSquaresToEdge[friendlyKingSquare][dirIndex]; i++) {
	//				int squareIndex = friendlyKingSquare + directionOffsets[dirIndex] * (i + 1);
	//				int piece = board.Square[squareIndex];
	//				if (piece != Piece.None) {
	//					// Friendly piece is blocking view of this square from the enemy.
	//					if (Piece.IsColour(piece, friendlyColour)) {
	//						break;
	//					}
	//					// This square contains an enemy piece
	//					else {
	//						if (Piece.IsRookOrQueen(piece)) {
	//							return true;
	//						}
	//						else {
	//							// This piece is not able to move in the current direction, and is therefore blocking any checks along this line
	//							break;
	//						}
	//					}
	//				}
	//			}
	//
	//			// check if enemy pawn is controlling this square (can't use pawn attack bitboard, because pawn has been captured)
	//			for (int i = 0; i < 2; i++) {
	//				// Check if square exists diagonal to friendly king from which enemy pawn could be attacking it
	//				if (numSquaresToEdge[friendlyKingSquare][pawnAttackDirections[friendlyColourIndex][i]] > 0) {
	//					// move in direction friendly pawns attack to get square from which enemy pawn would attack
	//					int piece = board.Square[friendlyKingSquare + directionOffsets[pawnAttackDirections[friendlyColourIndex][i]]];
	//					if (piece == (Piece.Pawn | opponentColour)) // is enemy pawn
	//					{
	//						return true;
	//					}
	//				}
	//			}
	//
	//			return false;
	//		}
	//	};
	//
	//	// Piece
	//	static class Piece {
	//	public:
	//		static const int None = 0;
	//		static const int King = 1;
	//		static const int Pawn = 2;
	//		static const int Knight = 3;
	//		static const int Bishop = 5;
	//		static const int Rook = 6;
	//		static const int Queen = 7;
	//
	//		static const int White = 8;
	//		static const int Black = 16;
	//
	//		static const int typeMask = 0b00111;
	//		static const int blackMask = 0b10000;
	//		static const int whiteMask = 0b01000;
	//		static const int colourMask = whiteMask | blackMask;
	//
	//		static bool IsColour(int piece, int colour) {
	//			return (piece & colourMask) == colour;
	//		}
	//
	//		static int Colour(int piece) {
	//			return piece & colourMask;
	//		}
	//
	//		static int PieceType(int piece) {
	//			return piece & typeMask;
	//		}
	//
	//		static bool IsRookOrQueen(int piece) {
	//			return (piece & 0b110) == 0b110;
	//		}
	//
	//		static bool IsBishopOrQueen(int piece) {
	//			return (piece & 0b101) == 0b101;
	//		}
	//
	//		static bool IsSlidingPiece(int piece) {
	//			return (piece & 0b100) != 0;
	//		}
	//	};
	//
	//	// Piece List
	//	class PieceList {
	//	public:
	//		// Indices of squares occupied by given piece type (only elements up to Count are valid, the rest are unused/garbage)
	//		int* occupiedSquares;
	//		// Map to go from index of a square, to the index in the occupiedSquares array where that square is stored
	//		int* map;
	//		int numPieces;
	//
	//		PieceList(int maxPieceCount = 16) {
	//			occupiedSquares = new int[maxPieceCount];
	//			map = new int[64];
	//			numPieces = 0;
	//		}
	//
	//		int Count() {
	//			return numPieces;
	//		}
	//
	//		void AddPieceAtSquare(int square) {
	//			occupiedSquares[numPieces] = square;
	//			map[square] = numPieces;
	//			numPieces++;
	//		}
	//
	//		void RemovePieceAtSquare(int square) {
	//			int pieceIndex = map[square]; // get the index of this element in the occupiedSquares array
	//			occupiedSquares[pieceIndex] = occupiedSquares[numPieces - 1]; // move last element in array to the place of the removed element
	//			map[occupiedSquares[pieceIndex]] = pieceIndex; // update map to point to the moved element's new location in the array
	//			numPieces--;
	//		}
	//
	//		void MovePiece(int startSquare, int targetSquare) {
	//			int pieceIndex = map[startSquare]; // get the index of this element in the occupiedSquares array
	//			occupiedSquares[pieceIndex] = targetSquare;
	//			map[targetSquare] = pieceIndex;
	//		}
	//
	//		//int this[int index] = > occupiedSquares[index];
	//
	//	};
	//
	//	// Piece Square Table
	//	static class PieceSquareTable {
	//
	//		static int Read(int* table, int square, bool isWhite) {
	//			if (isWhite) {
	//				int file = BoardRepresentation::FileIndex(square);
	//				int rank = BoardRepresentation::RankIndex(square);
	//				rank = 7 - rank;
	//				square = rank * 8 + file;
	//			}
	//
	//			return table[square];
	//		}
	//
	//		const int pawns[64] = {
	//			0,  0,  0,  0,  0,  0,  0,  0,
	//			50, 50, 50, 50, 50, 50, 50, 50,
	//			10, 10, 20, 30, 30, 20, 10, 10,
	//			5,  5, 10, 25, 25, 10,  5,  5,
	//			0,  0,  0, 20, 20,  0,  0,  0,
	//			5, -5,-10,  0,  0,-10, -5,  5,
	//			5, 10, 10,-20,-20, 10, 10,  5,
	//			0,  0,  0,  0,  0,  0,  0,  0
	//		};
	//
	//		const int knights[64] = {
	//			-50,-40,-30,-30,-30,-30,-40,-50,
	//			-40,-20,  0,  0,  0,  0,-20,-40,
	//			-30,  0, 10, 15, 15, 10,  0,-30,
	//			-30,  5, 15, 20, 20, 15,  5,-30,
	//			-30,  0, 15, 20, 20, 15,  0,-30,
	//			-30,  5, 10, 15, 15, 10,  5,-30,
	//			-40,-20,  0,  5,  5,  0,-20,-40,
	//			-50,-40,-30,-30,-30,-30,-40,-50,
	//		};
	//
	//		const int bishops[64] = {
	//			-20,-10,-10,-10,-10,-10,-10,-20,
	//			-10,  0,  0,  0,  0,  0,  0,-10,
	//			-10,  0,  5, 10, 10,  5,  0,-10,
	//			-10,  5,  5, 10, 10,  5,  5,-10,
	//			-10,  0, 10, 10, 10, 10,  0,-10,
	//			-10, 10, 10, 10, 10, 10, 10,-10,
	//			-10,  5,  0,  0,  0,  0,  5,-10,
	//			-20,-10,-10,-10,-10,-10,-10,-20,
	//		};
	//
	//		const int rooks[64] = {
	//			0,  0,  0,  0,  0,  0,  0,  0,
	//			5, 10, 10, 10, 10, 10, 10,  5,
	//			-5,  0,  0,  0,  0,  0,  0, -5,
	//			-5,  0,  0,  0,  0,  0,  0, -5,
	//			-5,  0,  0,  0,  0,  0,  0, -5,
	//			-5,  0,  0,  0,  0,  0,  0, -5,
	//			-5,  0,  0,  0,  0,  0,  0, -5,
	//			0,  0,  0,  5,  5,  0,  0,  0
	//		};
	//
	//		const int queens[64] = {
	//			-20,-10,-10, -5, -5,-10,-10,-20,
	//			-10,  0,  0,  0,  0,  0,  0,-10,
	//			-10,  0,  5,  5,  5,  5,  0,-10,
	//			-5,  0,  5,  5,  5,  5,  0, -5,
	//			0,  0,  5,  5,  5,  5,  0, -5,
	//			-10,  5,  5,  5,  5,  5,  0,-10,
	//			-10,  0,  5,  0,  0,  0,  0,-10,
	//			-20,-10,-10, -5, -5,-10,-10,-20
	//		};
	//
	//		const int kingMiddle[64] = {
	//			-30,-40,-40,-50,-50,-40,-40,-30,
	//			-30,-40,-40,-50,-50,-40,-40,-30,
	//			-30,-40,-40,-50,-50,-40,-40,-30,
	//			-30,-40,-40,-50,-50,-40,-40,-30,
	//			-20,-30,-30,-40,-40,-30,-30,-20,
	//			-10,-20,-20,-20,-20,-20,-20,-10,
	//			20, 20,  0,  0,  0,  0, 20, 20,
	//			20, 30, 10,  0,  0, 10, 30, 20
	//		};
	//
	//		const int kingEnd[64] = {
	//			-50,-40,-30,-20,-20,-30,-40,-50,
	//			-30,-20,-10,  0,  0,-10,-20,-30,
	//			-30,-10, 20, 30, 30, 20,-10,-30,
	//			-30,-10, 30, 40, 40, 30,-10,-30,
	//			-30,-10, 30, 40, 40, 30,-10,-30,
	//			-30,-10, 20, 30, 30, 20,-10,-30,
	//			-30,-30,  0,  0,  0,  0,-30,-30,
	//			-50,-30,-30,-30,-30,-30,-30,-50
	//		};
	//	};
	//
	//	// Player
	//	class Player {
	//		public event System.Action<Move> onMoveChosen;
	//
	//		void Update();
	//
	//		void NotifyTurnToMove();
	//
	//		protected virtual void ChoseMove(Move move) {
	//			onMoveChosen ? .Invoke(move);
	//		}
	//	};
	//
	//	// Precomputed Move Data
	//	static class PrecomputedMoveData {
	//		// First 4 are orthogonal, last 4 are diagonals (N, S, W, E, NW, SE, NE, SW)
	//		const int directionOffsets[8] = { 8, -8, -1, 1, 7, -7, 9, -9 };
	//
	//		// Stores number of moves available in each of the 8 directions for every square on the board
	//		// Order of directions is: N, S, W, E, NW, SE, NE, SW
	//		// So for example, if availableSquares[0][1] == 7...
	//		// that means that there are 7 squares to the north of b1 (the square with index 1 in board array)
	//		static int numSquaresToEdge[64][8];
	//
	//		// Stores array of indices for each square a knight can land on from any square on the board
	//		// So for example, knightMoves[0] is equal to {10, 17}, meaning a knight on a1 can jump to c2 and b3
	//		static std::vector<std::byte> knightMoves[64];
	//		static std::vector<std::byte> kingMoves[64];
	//
	//		// Pawn attack directions for white and black (NW, NE; SW SE)
	//		const int /*std::byte*/ pawnAttackDirections[2][2] = {
	//			{ 4, 6 },
	//			{ 7, 5 }
	//		};
	//
	//		static std::vector<int> pawnAttacksWhite[64];
	//		static std::vector<int> pawnAttacksBlack[64];
	//		static int directionLookup[127];
	//
	//		static unsigned long kingAttackBitboards[64];
	//		static unsigned long knightAttackBitboards[64];
	//		static unsigned long pawnAttackBitboards[64][2];
	//
	//		static unsigned long rookMoves[64];
	//		static unsigned long bishopMoves[64];
	//		static unsigned long queenMoves[64];
	//
	//		// Aka manhattan distance (answers how many moves for a rook to get from square a to square b)
	//		static int orthogonalDistance[64][64];
	//		// Aka chebyshev distance (answers how many moves for a king to get from square a to square b)
	//		static int kingDistance[64][64];
	//		static int centreManhattanDistance[64];
	//
	//		static int NumRookMovesToReachSquare(int startSquare, int targetSquare) {
	//			return orthogonalDistance[startSquare][targetSquare];
	//		}
	//
	//		static int NumKingMovesToReachSquare(int startSquare, int targetSquare) {
	//			return kingDistance[startSquare][targetSquare];
	//		}
	//
	//		// Initialize lookup data
	//		PrecomputedMoveData() {
	//
	//			// Calculate knight jumps and available squares for each square on the board.
	//			// See comments by variable definitions for more info.
	//			int allKnightJumps[8] = { 15, 17, -17, -15, 10, -6, 6, -10 };
	//
	//			for (int squareIndex = 0; squareIndex < 64; squareIndex++) {
	//
	//				int y = squareIndex / 8;
	//				int x = squareIndex - y * 8;
	//
	//				int north = 7 - y;
	//				int south = y;
	//				int west = x;
	//				int east = 7 - x;
	//				numSquaresToEdge[squareIndex][0] = north;
	//				numSquaresToEdge[squareIndex][1] = south;
	//				numSquaresToEdge[squareIndex][2] = west;
	//				numSquaresToEdge[squareIndex][3] = east;
	//				numSquaresToEdge[squareIndex][4] = std::min(north, west);
	//				numSquaresToEdge[squareIndex][5] = std::min(south, east);
	//				numSquaresToEdge[squareIndex][6] = std::min(north, east);
	//				numSquaresToEdge[squareIndex][7] = std::min(south, west);
	//
	//				// Calculate all squares knight can jump to from current square
	//				std::vector<std::byte> legalKnightJumps;
	//				unsigned long knightBitboard = 0;
	//				for(int knightJumpDelta : allKnightJumps) {
	//					int knightJumpSquare = squareIndex + knightJumpDelta;
	//					if (knightJumpSquare >= 0 && knightJumpSquare < 64) {
	//						int knightSquareY = knightJumpSquare / 8;
	//						int knightSquareX = knightJumpSquare - knightSquareY * 8;
	//						// Ensure knight has moved max of 2 squares on x/y axis (to reject indices that have wrapped around side of board)
	//						int maxCoordMoveDst = std::max(std::abs(x - knightSquareX), std::abs(y - knightSquareY));
	//						if (maxCoordMoveDst == 2) {
	//							legalKnightJumps.push_back((std::byte)knightJumpSquare);
	//							knightBitboard |= 1ul << knightJumpSquare;
	//						}
	//					}
	//				}
	//				knightMoves[squareIndex] = legalKnightJumps;
	//				knightAttackBitboards[squareIndex] = knightBitboard;
	//
	//				// Calculate all squares king can move to from current square (not including castling)
	//				std::vector<std::byte> legalKingMoves;
	//				for(int kingMoveDelta : directionOffsets) {
	//					int kingMoveSquare = squareIndex + kingMoveDelta;
	//					if (kingMoveSquare >= 0 && kingMoveSquare < 64) {
	//						int kingSquareY = kingMoveSquare / 8;
	//						int kingSquareX = kingMoveSquare - kingSquareY * 8;
	//						// Ensure king has moved max of 1 square on x/y axis (to reject indices that have wrapped around side of board)
	//						int maxCoordMoveDst = std::max(std::abs(x - kingSquareX), std::abs(y - kingSquareY));
	//						if (maxCoordMoveDst == 1) {
	//							legalKingMoves.push_back((std::byte)kingMoveSquare);
	//							kingAttackBitboards[squareIndex] |= 1ul << kingMoveSquare;
	//						}
	//					}
	//				}
	//				kingMoves[squareIndex] = legalKingMoves;
	//
	//				// Calculate legal pawn captures for white and black
	//				std::vector<int> pawnCapturesWhite;
	//				std::vector<int> pawnCapturesBlack;
	//				if (x > 0) {
	//					if (y < 7) {
	//						pawnCapturesWhite.push_back(squareIndex + 7);
	//						pawnAttackBitboards[squareIndex][WhiteIndex] |= 1ul << (squareIndex + 7);
	//					}
	//					if (y > 0) {
	//						pawnCapturesBlack.push_back(squareIndex - 9);
	//						pawnAttackBitboards[squareIndex][BlackIndex] |= 1ul << (squareIndex - 9);
	//					}
	//				}
	//				if (x < 7) {
	//					if (y < 7) {
	//						pawnCapturesWhite.push_back(squareIndex + 9);
	//						pawnAttackBitboards[squareIndex][WhiteIndex] |= 1ul << (squareIndex + 9);
	//					}
	//					if (y > 0) {
	//						pawnCapturesBlack.push_back(squareIndex - 7);
	//						pawnAttackBitboards[squareIndex][BlackIndex] |= 1ul << (squareIndex - 7);
	//					}
	//				}
	//				pawnAttacksWhite[squareIndex] = pawnCapturesWhite;
	//				pawnAttacksBlack[squareIndex] = pawnCapturesBlack;
	//
	//				// Rook moves
	//				for (int directionIndex = 0; directionIndex < 4; directionIndex++) {
	//					int currentDirOffset = directionOffsets[directionIndex];
	//					for (int n = 0; n < numSquaresToEdge[squareIndex][directionIndex]; n++) {
	//						int targetSquare = squareIndex + currentDirOffset * (n + 1);
	//						rookMoves[squareIndex] |= 1ul << targetSquare;
	//					}
	//				}
	//				// Bishop moves
	//				for (int directionIndex = 4; directionIndex < 8; directionIndex++) {
	//					int currentDirOffset = directionOffsets[directionIndex];
	//					for (int n = 0; n < numSquaresToEdge[squareIndex][directionIndex]; n++) {
	//						int targetSquare = squareIndex + currentDirOffset * (n + 1);
	//						bishopMoves[squareIndex] |= 1ul << targetSquare;
	//					}
	//				}
	//				queenMoves[squareIndex] = rookMoves[squareIndex] | bishopMoves[squareIndex];
	//			}
	//
	//			for (int i = 0; i < 127; i++) {
	//				int offset = i - 63;
	//				int absOffset = std::abs(offset);
	//				int absDir = 1;
	//				if (absOffset % 9 == 0) {
	//					absDir = 9;
	//				}
	//				else if (absOffset % 8 == 0) {
	//					absDir = 8;
	//				}
	//				else if (absOffset % 7 == 0) {
	//					absDir = 7;
	//				}
	//
	//				directionLookup[i] = absDir * glm::sign(offset);
	//			}
	//
	//			// Distance lookup
	//			for (int squareA = 0; squareA < 64; squareA++) {
	//				Coord coordA = BoardRepresentation::CoordFromIndex(squareA);
	//				int fileDstFromCentre = std::max(3 - coordA.fileIndex, coordA.fileIndex - 4);
	//				int rankDstFromCentre = std::max(3 - coordA.rankIndex, coordA.rankIndex - 4);
	//				centreManhattanDistance[squareA] = fileDstFromCentre + rankDstFromCentre;
	//
	//				for (int squareB = 0; squareB < 64; squareB++) {
	//
	//					Coord coordB = BoardRepresentation::CoordFromIndex(squareB);
	//					int rankDistance = std::abs(coordA.rankIndex - coordB.rankIndex);
	//					int fileDistance = std::abs(coordA.fileIndex - coordB.fileIndex);
	//					orthogonalDistance[squareA][squareB] = fileDistance + rankDistance;
	//					kingDistance[squareA][squareB] = std::max(fileDistance, rankDistance);
	//				}
	//			}
	//		}
	//	};
	//
	//	// Pseudo Legal Move Generator
	//	class PseudoLegalMoveGenerator {
	//	public:
	//		// ---- Instance variables ----
	//		std::vector<Move> moves;
	//		bool isWhiteToMove;
	//		int friendlyColour;
	//		int opponentColour;
	//		int friendlyKingSquare;
	//		int friendlyColourIndex;
	//		int opponentColourIndex;
	//
	//		bool genQuiets;
	//		bool genUnderpromotions;
	//		Board board;
	//
	//		// Generates list of legal moves in current position.
	//		// Quiet moves (non captures) can optionally be excluded. This is used in quiescence search.
	//		std::vector<Move> GenerateMoves(Board boardIn, bool includeQuietMoves = true, bool includeUnderPromotions = true) {
	//			board = boardIn;
	//			genQuiets = includeQuietMoves;
	//			genUnderpromotions = includeUnderPromotions;
	//			Init();
	//			GenerateKingMoves();
	//
	//			GenerateSlidingMoves();
	//			GenerateKnightMoves();
	//			GeneratePawnMoves();
	//
	//			return moves;
	//		}
	//
	//		bool Illegal() {
	//			return SquareAttacked(board.KingSquare[1 - board.ColourToMoveIndex], board.ColourToMove);
	//		}
	//
	//		bool SquareAttacked(int attackSquare, int attackerColour) {
	//
	//			int attackerColourIndex = (attackerColour == Piece::White) ? WhiteIndex : BlackIndex;
	//			int friendlyColourIndex = 1 - attackerColourIndex;
	//			int friendlyColour = (attackerColour == Piece::White) ? Piece::Black : Piece::White;
	//
	//			int startDirIndex = 0;
	//			int endDirIndex = 8;
	//
	//			int opponentKingSquare = board.KingSquare[attackerColourIndex];
	//			if (kingDistance[opponentKingSquare, attackSquare] == 1) {
	//				return true;
	//			}
	//
	//			if (board.queens[attackerColourIndex].Count == 0) {
	//				startDirIndex = (board.rooks[attackerColourIndex].Count > 0) ? 0 : 4;
	//				endDirIndex = (board.bishops[attackerColourIndex].Count > 0) ? 8 : 4;
	//			}
	//
	//			for (int dir = startDirIndex; dir < endDirIndex; dir++) {
	//				bool isDiagonal = dir > 3;
	//
	//				int n = numSquaresToEdge[attackSquare][dir];
	//				int directionOffset = directionOffsets[dir];
	//
	//				for (int i = 0; i < n; i++) {
	//					int squareIndex = attackSquare + directionOffset * (i + 1);
	//					int piece = board.Square[squareIndex];
	//
	//					// This square contains a piece
	//					if (piece != Piece::None) {
	//						if (Piece::IsColour(piece, friendlyColour)) {
	//							break;
	//						}
	//						// This square contains an enemy piece
	//						else {
	//							int pieceType = Piece::PieceType(piece);
	//
	//							// Check if piece is in bitmask of pieces able to move in current direction
	//							if (isDiagonal && Piece::IsBishopOrQueen(pieceType) || !isDiagonal && Piece::IsRookOrQueen(pieceType)) {
	//								return true;
	//							}
	//							else {
	//								// This enemy piece is not able to move in the current direction, and so is blocking any checks/pins
	//								break;
	//							}
	//						}
	//					}
	//				}
	//			}
	//
	//			// Knight attacks
	//			aut knightAttackSquares = knightMoves[attackSquare];
	//			for (int i = 0; i < knightAttackSquares.Length; i++) {
	//				if (board.Square[knightAttackSquares[i]] == (Piece.Knight | attackerColour)) {
	//					return true;
	//				}
	//			}
	//
	//			// check if enemy pawn is controlling this square
	//			for (int i = 0; i < 2; i++) {
	//				// Check if square exists diagonal to friendly king from which enemy pawn could be attacking it
	//				if (numSquaresToEdge[attackSquare][pawnAttackDirections[friendlyColourIndex][i]] > 0) {
	//					// move in direction friendly pawns attack to get square from which enemy pawn would attack
	//					int s = attackSquare + directionOffsets[pawnAttackDirections[friendlyColourIndex][i]];
	//
	//					int piece = board.Square[s];
	//					if (piece == (Piece.Pawn | attackerColour)) // is enemy pawn
	//					{
	//						return true;
	//					}
	//				}
	//			}
	//
	//			return false;
	//		}
	//
	//		// Note, this will only return correct value after GenerateMoves() has been called in the current position
	//		public bool InCheck() {
	//			return false;
	//			//return SquareAttacked (friendlyKingSquare, board.ColourToMoveIndex);
	//		}
	//
	//		void Init() {
	//			moves = new List<Move>(64);
	//
	//			isWhiteToMove = board.ColourToMove == Piece.White;
	//			friendlyColour = board.ColourToMove;
	//			opponentColour = board.OpponentColour;
	//			friendlyKingSquare = board.KingSquare[board.ColourToMoveIndex];
	//			friendlyColourIndex = (board.WhiteToMove) ? Board.WhiteIndex : Board.BlackIndex;
	//			opponentColourIndex = 1 - friendlyColourIndex;
	//		}
	//
	//		void GenerateKingMoves() {
	//			for (int i = 0; i < kingMoves[friendlyKingSquare].Length; i++) {
	//				int targetSquare = kingMoves[friendlyKingSquare][i];
	//				int pieceOnTargetSquare = board.Square[targetSquare];
	//
	//				// Skip squares occupied by friendly pieces
	//				if (Piece.IsColour(pieceOnTargetSquare, friendlyColour)) {
	//					continue;
	//				}
	//
	//				bool isCapture = Piece.IsColour(pieceOnTargetSquare, opponentColour);
	//				if (!isCapture) {
	//					// King can't move to square marked as under enemy control, unless he is capturing that piece
	//					// Also skip if not generating quiet moves
	//					if (!genQuiets) {
	//						continue;
	//					}
	//				}
	//
	//				// Safe for king to move to this square
	//
	//				moves.Add(new Move(friendlyKingSquare, targetSquare));
	//
	//				// Castling:
	//				if (!isCapture && !SquareAttacked(friendlyKingSquare, opponentColour)) {
	//					// Castle kingside
	//					if ((targetSquare == f1 || targetSquare == f8) && HasKingsideCastleRight) {
	//						if (!SquareAttacked(targetSquare, opponentColour)) {
	//							int castleKingsideSquare = targetSquare + 1;
	//							if (board.Square[castleKingsideSquare] == Piece.None) {
	//								moves.Add(new Move(friendlyKingSquare, castleKingsideSquare, Move.Flag.Castling));
	//
	//							}
	//						}
	//					}
	//					// Castle queenside
	//					else if ((targetSquare == d1 || targetSquare == d8) && HasQueensideCastleRight) {
	//						if (!SquareAttacked(targetSquare, opponentColour)) {
	//							int castleQueensideSquare = targetSquare - 1;
	//							if (board.Square[castleQueensideSquare] == Piece.None && board.Square[castleQueensideSquare - 1] == Piece.None) {
	//								moves.Add(new Move(friendlyKingSquare, castleQueensideSquare, Move.Flag.Castling));
	//							}
	//						}
	//					}
	//				}
	//
	//			}
	//		}
	//
	//		void GenerateSlidingMoves() {
	//			PieceList rooks = board.rooks[friendlyColourIndex];
	//			for (int i = 0; i < rooks.Count; i++) {
	//				GenerateSlidingPieceMoves(rooks[i], 0, 4);
	//			}
	//
	//			PieceList bishops = board.bishops[friendlyColourIndex];
	//			for (int i = 0; i < bishops.Count; i++) {
	//				GenerateSlidingPieceMoves(bishops[i], 4, 8);
	//			}
	//
	//			PieceList queens = board.queens[friendlyColourIndex];
	//			for (int i = 0; i < queens.Count; i++) {
	//				GenerateSlidingPieceMoves(queens[i], 0, 8);
	//			}
	//
	//		}
	//
	//		void GenerateSlidingPieceMoves(int startSquare, int startDirIndex, int endDirIndex) {
	//
	//			for (int directionIndex = startDirIndex; directionIndex < endDirIndex; directionIndex++) {
	//				int currentDirOffset = directionOffsets[directionIndex];
	//
	//				for (int n = 0; n < numSquaresToEdge[startSquare][directionIndex]; n++) {
	//					int targetSquare = startSquare + currentDirOffset * (n + 1);
	//					int targetSquarePiece = board.Square[targetSquare];
	//
	//					// Blocked by friendly piece, so stop looking in this direction
	//					if (Piece.IsColour(targetSquarePiece, friendlyColour)) {
	//						break;
	//					}
	//					bool isCapture = targetSquarePiece != Piece.None;
	//
	//					if (genQuiets || isCapture) {
	//						moves.Add(new Move(startSquare, targetSquare));
	//					}
	//
	//					// If square not empty, can't move any further in this direction
	//					// Also, if this move blocked a check, further moves won't block the check
	//					if (isCapture) {
	//						break;
	//					}
	//				}
	//			}
	//		}
	//
	//		void GenerateKnightMoves() {
	//			PieceList myKnights = board.knights[friendlyColourIndex];
	//
	//			for (int i = 0; i < myKnights.Count; i++) {
	//				int startSquare = myKnights[i];
	//
	//				for (int knightMoveIndex = 0; knightMoveIndex < knightMoves[startSquare].Length; knightMoveIndex++) {
	//					int targetSquare = knightMoves[startSquare][knightMoveIndex];
	//					int targetSquarePiece = board.Square[targetSquare];
	//					bool isCapture = Piece.IsColour(targetSquarePiece, opponentColour);
	//					if (genQuiets || isCapture) {
	//						// Skip if square contains friendly piece, or if in check and knight is not interposing/capturing checking piece
	//						if (Piece.IsColour(targetSquarePiece, friendlyColour)) {
	//							continue;
	//						}
	//						moves.Add(new Move(startSquare, targetSquare));
	//					}
	//				}
	//			}
	//		}
	//
	//		void GeneratePawnMoves() {
	//			PieceList myPawns = board.pawns[friendlyColourIndex];
	//			int pawnOffset = (friendlyColour == Piece.White) ? 8 : -8;
	//			int startRank = (board.WhiteToMove) ? 1 : 6;
	//			int finalRankBeforePromotion = (board.WhiteToMove) ? 6 : 1;
	//
	//			int enPassantFile = ((int)(board.currentGameState >> 4) & 15) - 1;
	//			int enPassantSquare = -1;
	//			if (enPassantFile != -1) {
	//				enPassantSquare = 8 * ((board.WhiteToMove) ? 5 : 2) + enPassantFile;
	//			}
	//
	//			for (int i = 0; i < myPawns.Count; i++) {
	//				int startSquare = myPawns[i];
	//				int rank = RankIndex(startSquare);
	//				bool oneStepFromPromotion = rank == finalRankBeforePromotion;
	//
	//				if (genQuiets) {
	//
	//					int squareOneForward = startSquare + pawnOffset;
	//
	//					// Square ahead of pawn is empty: forward moves
	//					if (board.Square[squareOneForward] == Piece.None) {
	//						// Pawn not pinned, or is moving along line of pin
	//
	//						if (oneStepFromPromotion) {
	//							MakePromotionMoves(startSquare, squareOneForward);
	//						}
	//						else {
	//							moves.Add(new Move(startSquare, squareOneForward));
	//						}
	//
	//						// Is on starting square (so can move two forward if not blocked)
	//						if (rank == startRank) {
	//							int squareTwoForward = squareOneForward + pawnOffset;
	//							if (board.Square[squareTwoForward] == Piece.None) {
	//								// Not in check, or pawn is interposing checking piece
	//
	//								moves.Add(new Move(startSquare, squareTwoForward, Move.Flag.PawnTwoForward));
	//
	//							}
	//						}
	//
	//					}
	//				}
	//
	//				// Pawn captures.
	//				for (int j = 0; j < 2; j++) {
	//					// Check if square exists diagonal to pawn
	//					if (numSquaresToEdge[startSquare][pawnAttackDirections[friendlyColourIndex][j]] > 0) {
	//						// move in direction friendly pawns attack to get square from which enemy pawn would attack
	//						int pawnCaptureDir = directionOffsets[pawnAttackDirections[friendlyColourIndex][j]];
	//						int targetSquare = startSquare + pawnCaptureDir;
	//						int targetPiece = board.Square[targetSquare];
	//
	//						// Regular capture
	//						if (Piece.IsColour(targetPiece, opponentColour)) {
	//
	//							if (oneStepFromPromotion) {
	//								MakePromotionMoves(startSquare, targetSquare);
	//							}
	//							else {
	//								moves.Add(new Move(startSquare, targetSquare));
	//							}
	//						}
	//
	//						// Capture en-passant
	//						if (targetSquare == enPassantSquare) {
	//							int epCapturedPawnSquare = targetSquare + ((board.WhiteToMove) ? -8 : 8);
	//
	//							moves.Add(new Move(startSquare, targetSquare, Move.Flag.EnPassantCapture));
	//
	//						}
	//					}
	//				}
	//			}
	//		}
	//
	//		void MakePromotionMoves(int fromSquare, int toSquare) {
	//			moves.Add(new Move(fromSquare, toSquare, Move.Flag.PromoteToQueen));
	//			if (genUnderpromotions) {
	//				moves.Add(new Move(fromSquare, toSquare, Move.Flag.PromoteToKnight));
	//				moves.Add(new Move(fromSquare, toSquare, Move.Flag.PromoteToRook));
	//				moves.Add(new Move(fromSquare, toSquare, Move.Flag.PromoteToBishop));
	//			}
	//		}
	//
	//		bool HasKingsideCastleRight{
	//			get {
	//				int mask = (board.WhiteToMove) ? 1 : 4;
	//				return (board.currentGameState & mask) != 0;
	//			}
	//		}
	//
	//			bool HasQueensideCastleRight{
	//				get {
	//					int mask = (board.WhiteToMove) ? 2 : 8;
	//					return (board.currentGameState & mask) != 0;
	//				}
	//			}
	//
	//	}
	//}
}
