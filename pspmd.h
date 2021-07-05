/*-----------------------------------------------------------------------------
 * Text console Mouse Daemon for uClinux on PSP
 * Created by Jackson Mo, Jan 29, 2008
 *---------------------------------------------------------------------------*/
#include <stdio.h>


//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------
#if 1
#define DEBUG       1
#endif

#ifdef DEBUG
#define DBG(args)   printf args
#define DBG_PREFIX  "MD: "
#else
#define DBG(args)
#define DBG_PREFIX
#endif


//-----------------------------------------------------------------------------
// Classes
//-----------------------------------------------------------------------------
class PspMdConsole;
class PspMdScreen;
class PspMdMouse;
class PspMouseDaemon;


//-----------------------------------------------------------------------------
// Class: PspMdConsole
//-----------------------------------------------------------------------------
class PspMdConsole
{
public:
  PspMdConsole(PspMouseDaemon & md_);
  virtual ~PspMdConsole();

  bool Initialize();
  bool Seek(unsigned int pos_);
  bool Read(void * buf_, unsigned int size_, unsigned int & bytesRead_);
  bool Paste(const char * str_);

  int GetCols() const { return m_cols; }
  int GetRows() const { return m_rows; }

protected:
  PspMouseDaemon & m_md;
  int m_vcsFd;
  int m_cols;
  int m_rows;

private:
  // Not implemented
  PspMdConsole();
  PspMdConsole(const PspMdConsole &);
  PspMdConsole & operator = (const PspMdConsole &);
};


//-----------------------------------------------------------------------------
// Class: PspMdScreen
//-----------------------------------------------------------------------------
class PspMdScreen
{
public:
  PspMdScreen(PspMouseDaemon & md_);
  virtual ~PspMdScreen();

  bool Initialize();
  bool Sync();
  bool Xor(int x_, int y_, int width_, int height_, unsigned int code_);

  unsigned int * GetAddress() const { return m_vramBase; }
  unsigned int GetSize() const  { return m_vramSize; }
  int GetWidth() const          { return m_width; }
  int GetHeight() const         { return m_height; }
  int GetVirtualWidth() const   { return m_virtualWidth; }
  int GetVirtualHeight() const  { return m_virtualHeight; }

protected:
  PspMouseDaemon & m_md;
  int m_fbFd;
  unsigned int * m_vramBase;
  unsigned int m_vramSize;
  int m_width;
  int m_height;
  int m_virtualWidth;
  int m_virtualHeight;

private:
  // Not implemented
  PspMdScreen();
  PspMdScreen(const PspMdScreen &);
  PspMdScreen & operator = (const PspMdScreen &);
};


//-----------------------------------------------------------------------------
// Class: PspMdMouse
//-----------------------------------------------------------------------------
class PspMdMouse
{
public:
  PspMdMouse(PspMouseDaemon & md_);
  virtual ~PspMdMouse();

  bool Initialize();
  bool SetRegion(int left_, int top_, int right_, int bottom_);
  bool Poll();
  void SetPos(int x_, int y_);

  bool GetLeft() const  { return m_left; }
  bool GetMid() const   { return m_mid; }
  bool GetRight() const { return m_right; }
  int GetX() const      { return m_x; }
  int GetY() const      { return m_y; }

protected:
  PspMouseDaemon & m_md;
  int m_mouseFd;
  bool m_left;
  bool m_mid;
  bool m_right;
  int m_x;
  int m_y;
  int m_regionLeft;
  int m_regionTop;
  int m_regionRight;
  int m_regionBottom;

private:
  // Not implemented
  PspMdMouse();
  PspMdMouse(const PspMdMouse &);
  PspMdMouse & operator = (const PspMdMouse &);
};


//-----------------------------------------------------------------------------
// Class: PspMouseDaemon
//-----------------------------------------------------------------------------
class PspMouseDaemon
{
public:
  PspMouseDaemon();
  virtual ~PspMouseDaemon();

  bool Initialize();
  bool Run();

protected:
  // Internal states
  class BaseState;
    class FailedState;
    class CursorState;
    class HighlightState;

  #define PSPMD_STATES_H
  #include "pspmdstates.h"
  #undef  PSPMD_STATES_H

  void changeState(BaseState * newState_);
  void screenToConsole(int x_, int y_, int & col_, int & row_);
  void consoleToLinear(int col_, int row_, int & pos_);
  void linearToConsole(int pos_, int & col_, int & row_);

  bool draw(int col_, int row_, bool cursor_, bool highlight_);
  bool clear(int col_, int row_, bool cursor_, bool highlight_);
  bool sync();
  bool copyCb(int begin_, int end_);
  bool pasteCb();
  bool clearCb();

  PspMdConsole    m_console;
  PspMdScreen     m_screen;
  PspMdMouse      m_mouse;

  int             m_colWidth;
  int             m_rowHeight;
  int             m_col;
  int             m_row;
  char *          m_clipboardBuf;
  int             m_clipboardSize;

  BaseState *     m_currentState;
  FailedState     m_failedState;
  CursorState     m_cursorState;
  HighlightState  m_highlightState;



private:
  // Not implemented
  PspMouseDaemon(const PspMouseDaemon &);
  PspMouseDaemon & operator = (const PspMouseDaemon &);

  friend class BaseState;
  friend class FailedState;
  friend class CursorState;
  friend class HighlightState;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
