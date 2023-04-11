#pragma once
#include "Dymatic/Events/MouseEvent.h"

namespace Dymatic {

	class ProfilerPanel
	{
	public:
		ProfilerPanel();
		
		void OnImGuiRender();
		
	public:
		enum DataFormat
		{
			DataFormat_Bin = 0,
			DataFormat_Dec = 1,
			DataFormat_Hex = 2,
			DataFormat_COUNT
		};
		
	private:		
		// Timer
		double m_TimerViewStart = 0.0f;
		double m_TimerViewEnd = 10.0 * 1e+6;
		bool m_TimerRecording = false;
		
		// Memory Editor
		struct Sizes
		{
			int     AddrDigitsCount;
			float   LineHeight;
			float   GlyphWidth;
			float   HexCellWidth;
			float   SpacingBetweenMidCols;
			float   PosHexStart;
			float   PosHexEnd;
			float   PosAsciiStart;
			float   PosAsciiEnd;
			float   WindowWidth;

			Sizes();
		};
		
		void DrawProfileTimers();
		
		void DrawMemoryEditor();
		void GotoAddrAndHighlight(size_t addr_min, size_t addr_max);
		void CalcSizes(Sizes& s, size_t mem_size, size_t base_display_addr);
		void DrawOptionsLine(const Sizes& s, void* mem_data, size_t mem_size, size_t base_display_addr);
		void DrawPreviewLine(const Sizes& s, void* mem_data_void, size_t mem_size, size_t base_display_addr, size_t visible_start_addr);
		void* EndianessCopy(void* dst, void* src, size_t size) const;
		void DrawPreviewData(size_t addr, const unsigned char* mem_data, size_t mem_size, int data_type, DataFormat data_format, char* out_buf, size_t out_buf_size) const;
	
	private:
		// Memory Editor Settings
		bool            Open;                      
		bool            ReadOnly;                  
		int             Cols;                      
		bool            OptShowOptions;            
		bool            OptShowDataPreview;        
		bool            OptShowHexII;              
		bool            OptShowAscii;              
		bool            OptGreyOutZeroes;          
		bool            OptUpperCaseHex;           
		int             OptMidColsCount;           
		int             OptAddrDigitsCount;        
		float           OptFooterExtraHeight;      
		unsigned int    HighlightColor;

		// Memory Editor internal states
		bool            ContentsWidthChanged;
		size_t          DataPreviewAddr;
		size_t          DataEditingAddr;
		bool            DataEditingTakeFocus;
		char            DataInputBuf[32];
		char            AddrInputBuf[32];
		size_t          GotoAddr;
		size_t          HighlightMin, HighlightMax;
		int             PreviewEndianess;
		int				PreviewDataType;
	
	};

}
