/*-----------------------------------------------------------------------------
 * Text console Mouse Daemon for uClinux on PSP
 * Created by Jackson Mo, Jan 29, 2008
 *---------------------------------------------------------------------------*/
#include "pspmd.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>


//-----------------------------------------------------------------------------
// Type definitions
//-----------------------------------------------------------------------------
typedef struct
{
  unsigned int cols;
  unsigned int rows;
} psp_vcs_size_t;


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
static const char c_vcsDevName[]          = "/dev/vcs";
static const char c_fbDevName[]           = "/dev/fb";
static const char c_mouseDevName[]        = "/dev/mouse";
static const int  c_mouseInfoSize         = 3;

static const int INVALID_FD               = -1;
static const int PSP_VCS_IOCTL_PUTCHAR    = 101;
static const int PSP_VCS_IOCTL_GET_SIZE   = 109;

static const unsigned int c_mouseBtnMask  = 0x7;
static const unsigned int c_mouseBtnLeft  = 0x1;
static const unsigned int c_mouseBtnMid   = 0x4;
static const unsigned int c_mouseBtnRight = 0x2;

static const unsigned int c_cursorMask    = 0x01000000;
static const unsigned int c_highlightMask = 0x02000000;
static const unsigned int c_drawColor     = 0x00aaaaaa;
static const unsigned int c_cursorColor    = ( c_cursorMask | c_drawColor );
static const unsigned int c_highlightColor = ( c_highlightMask | c_drawColor );

static const unsigned int c_failureDelay  = 1;  // 1 second


//-----------------------------------------------------------------------------
// Class: PspMdConsole
//-----------------------------------------------------------------------------
PspMdConsole::PspMdConsole(PspMouseDaemon & md_)
  : m_md( md_ ),
    m_vcsFd( INVALID_FD ),
    m_cols( 0 ),
    m_rows( 0 )
{
}
//-----------------------------------------------------------------------------
PspMdConsole::~PspMdConsole()
{
  if ( m_vcsFd >= 0 )
  {
    (void)close( m_vcsFd );
    m_vcsFd = INVALID_FD;
  }
}
//-----------------------------------------------------------------------------
bool PspMdConsole::Initialize()
{
  if ( m_vcsFd >= 0 )
  {
    DBG(( DBG_PREFIX "PspMdConsole has been initialized\n" ));
    return true;
  }

  m_vcsFd = open( c_vcsDevName, O_RDONLY );
  if ( m_vcsFd < 0 )
  {
    DBG(( DBG_PREFIX "Failed to open vcs driver, err=%d\n", m_vcsFd ));
    return false;
  }

  psp_vcs_size_t sz;
  int rt = ioctl( m_vcsFd, PSP_VCS_IOCTL_GET_SIZE, (int)&sz );
  if ( rt < 0 )
  {
    DBG(( DBG_PREFIX "Failed to obtain vcs size, err=%d\n", rt ));
    return false;
  }

  m_cols = (int)sz.cols;
  m_rows = (int)sz.rows;

  return true;
}
//-----------------------------------------------------------------------------
bool PspMdConsole::Seek(unsigned int pos_)
{
  if ( m_vcsFd < 0 )
  {
    DBG(( DBG_PREFIX "Tried to seek an invalid vcs device\n" ));
    return false;
  }

  if ( pos_ >= m_cols * m_rows )
  {
    DBG(( DBG_PREFIX "Tried to seek outside vcs\n" ));
    return false;
  }

  int rt = lseek( m_vcsFd, pos_, SEEK_SET );
  if ( rt < 0 || rt != pos_ )
  {
    DBG(( DBG_PREFIX "Failed to seek vcs, err=%d\n", rt ));
    return false;
  }

  return true;
}
//-----------------------------------------------------------------------------
bool PspMdConsole::Read
(
  void * buf_,
  unsigned int size_,
  unsigned int & bytesRead_)
{
  bytesRead_ = 0;

  if ( m_vcsFd < 0 )
  {
    DBG(( DBG_PREFIX "Tried to read an invalid vcs device\n" ));
    return false;
  }

  if ( buf_ == 0 || size_ == 0 )
  {
    DBG(( DBG_PREFIX "Invalid buffer for reading the vcs device\n" ));
    return false;
  }

  int rt = read( m_vcsFd, buf_, size_ );
  if ( rt < 0 )
  {
    DBG(( DBG_PREFIX "Failed to read vcs device, err=%d\n", rt ));
    return false;
  }

  bytesRead_ = (unsigned int)rt;
  return true;
}
//-----------------------------------------------------------------------------
bool PspMdConsole::Paste(const char * str_)
{
  for ( ; *str_ != 0; str_++ )
  {
    int rt = ioctl( m_vcsFd, PSP_VCS_IOCTL_PUTCHAR, (int)( *str_ ) );
    if ( rt < 0 )
    {
      DBG(( DBG_PREFIX "Failed to paste to console, err=%d\n", rt ));
      return false;
    }
  }

  return true;
}


//-----------------------------------------------------------------------------
// Class: PspMdScreen
//-----------------------------------------------------------------------------
PspMdScreen::PspMdScreen(PspMouseDaemon & md_)
  : m_md( md_ ),
    m_fbFd( INVALID_FD ),
    m_vramBase( NULL ),
    m_vramSize( 0 ),
    m_width( 0 ),
    m_height( 0 ),
    m_virtualWidth( 0 ),
    m_virtualHeight( 0 )
{
}
//-----------------------------------------------------------------------------
PspMdScreen::~PspMdScreen()
{
  if ( m_fbFd >= 0 )
  {
    if ( m_vramBase != NULL && m_vramSize != 0 )
    {
      (void)munmap( m_vramBase, m_vramSize );
      m_vramBase = NULL;
      m_vramSize = 0;
    }

    (void)close( m_fbFd );
    m_fbFd = INVALID_FD;
  }
}
//-----------------------------------------------------------------------------
bool PspMdScreen::Initialize()
{
  if ( m_fbFd >= 0 )
  {
    DBG(( DBG_PREFIX "PspMdScreen has been initialized\n" ));
    return true;
  }

  m_fbFd = open( c_fbDevName, O_RDWR );
  if ( m_fbFd < 0 )
  {
    DBG(( DBG_PREFIX "Failed to open framebuffer driver, err=%d\n", m_fbFd ));
    return false;
  }

  struct fb_var_screeninfo vinfo;
  int rt = ioctl( m_fbFd, FBIOGET_VSCREENINFO, &vinfo );
  if ( rt < 0 )
  {
    DBG(( DBG_PREFIX "Failed to obtain framebuffer info, err=%d\n", rt ));
    return false;
  }

  m_vramSize = vinfo.xres_virtual *
               vinfo.yres_virtual *
               ( vinfo.bits_per_pixel >> 3 );
  m_width = vinfo.xres;
  m_height = vinfo.yres;
  m_virtualWidth = vinfo.xres_virtual;
  m_virtualHeight = vinfo.yres_virtual;

  m_vramBase = (unsigned int *)mmap( NULL,
                                     m_vramSize,
                                     PROT_READ | PROT_WRITE,
                                     MAP_SHARED,
                                     m_fbFd,
                                     0 );
  if ( (unsigned int)m_vramBase == (unsigned int)( -1 ) )
  {
    DBG(( DBG_PREFIX "Failed to map framebuffer memory, err=%d\n", m_vramBase ));
    m_vramBase = NULL;
    m_vramSize = 0;
    return false;
  }

  return true;
}
//-----------------------------------------------------------------------------
bool PspMdScreen::Sync()
{
  if ( m_fbFd < 0 )
  {
    DBG(( DBG_PREFIX "Tried to sync an invalid device\n" ));
    return false;
  }

  return ( fsync( m_fbFd ) == 0 );
}
//-----------------------------------------------------------------------------
bool PspMdScreen::Xor
(
  int x_,
  int y_,
  int width_,
  int height_,
  unsigned int code_
)
{
  if ( m_vramBase == NULL )
  {
    DBG(( DBG_PREFIX "Tried to draw on an invalid device\n" ));
    return false;
  }

  unsigned int * p = m_vramBase + x_ + y_ * m_virtualWidth;
  for ( int i = 0; i < height_; i++ )
  {
    for ( int j = 0; j < width_; j++ )
      p[ j ] ^= code_;

    p += m_virtualWidth;
  }

  return true;
}


//-----------------------------------------------------------------------------
// Class: PspMdMouse
//-----------------------------------------------------------------------------
PspMdMouse::PspMdMouse(PspMouseDaemon & md_)
  : m_md( md_ ),
    m_mouseFd( INVALID_FD ),
    m_left( false ),
    m_mid( false ),
    m_right( false ),
    m_x( 0 ),
    m_y( 0 ),
    m_regionLeft( 0 ),
    m_regionTop( 0 ),
    m_regionRight( 0 ),
    m_regionBottom( 0 )
{
}
//-----------------------------------------------------------------------------
PspMdMouse::~PspMdMouse()
{
  if ( m_mouseFd >= 0 )
  {
    (void)close( m_mouseFd );
    m_mouseFd = INVALID_FD;
  }
}
//-----------------------------------------------------------------------------
bool PspMdMouse::Initialize()
{
  if ( m_mouseFd >= 0 )
  {
    DBG(( DBG_PREFIX "PspMdMouse has been initialized\n" ));
    return true;
  }

  m_mouseFd = open( c_mouseDevName, O_RDONLY );
  if ( m_mouseFd < 0 )
  {
    DBG(( DBG_PREFIX "Failed to open mouse driver, err=%d\n", m_mouseFd ));
    return false;
  }

  return true;
}
//-----------------------------------------------------------------------------
bool PspMdMouse::SetRegion(int left_, int top_, int right_, int bottom_)
{
  if ( right_ < left_ || bottom_ < top_ )
  {
    DBG(( DBG_PREFIX "Invalid mouse region\n" ));
    return false;
  }

  m_regionLeft   = left_;
  m_regionTop    = top_;
  m_regionRight  = right_;
  m_regionBottom = bottom_;

  m_x = ( m_regionLeft + m_regionRight ) / 2;
  m_y = ( m_regionTop + m_regionBottom ) / 2;

  return true;
}
//-----------------------------------------------------------------------------
bool PspMdMouse::Poll()
{
  if ( m_mouseFd < 0 )
  {
    DBG(( DBG_PREFIX "Tried to poll an invalid mouse device\n" ));
    return false;
  }

  // read is blocked until a mouse event is generated
  char info[ c_mouseInfoSize ];
  int rt = read( m_mouseFd, info, c_mouseInfoSize );
  if ( rt != c_mouseInfoSize )
  {
    DBG(( DBG_PREFIX "Failed to read from mouse device, err=%d\n", rt ));
    return false;
  }

  unsigned int buttons = ( (unsigned int)( info[ 0 ] ) & c_mouseBtnMask );
  m_left  = ( buttons & c_mouseBtnLeft );
  m_mid   = ( buttons & c_mouseBtnMid );
  m_right = ( buttons & c_mouseBtnRight );

  SetPos( m_x + (int)info[ 1 ],
          m_y + (int)info[ 2 ] );

  return true;
}
//-----------------------------------------------------------------------------
void PspMdMouse::SetPos(int x_, int y_)
{
  if ( x_ < m_regionLeft )
    m_x = m_regionLeft;
  else if ( x_ > m_regionRight )
    m_x = m_regionRight;
  else
    m_x = x_;

  if ( y_ < m_regionTop )
    m_y = m_regionTop;
  else if ( y_ > m_regionBottom )
    m_y = m_regionBottom;
  else
    m_y = y_;
}


//-----------------------------------------------------------------------------
// Class: PspMouseDaemon
//-----------------------------------------------------------------------------
PspMouseDaemon::PspMouseDaemon()
  : m_console( *this ),
    m_screen( *this ),
    m_mouse( *this ),
    m_colWidth( 1 ),
    m_rowHeight( 1 ),
    m_col( 0 ),
    m_row( 0 ),
    m_clipboardBuf( NULL ),
    m_clipboardSize( 0 ),
    m_currentState( NULL ),
    m_failedState( *this ),
    m_cursorState( *this ),
    m_highlightState( *this )
{
}
//-----------------------------------------------------------------------------
PspMouseDaemon::~PspMouseDaemon()
{
  if ( m_clipboardBuf != NULL )
  {
    delete[] m_clipboardBuf;
    m_clipboardBuf = NULL;
    m_clipboardSize = 0;
  }
}
//-----------------------------------------------------------------------------
bool PspMouseDaemon::Initialize()
{
  if ( !m_console.Initialize() ||
       !m_screen.Initialize() ||
       !m_mouse.Initialize() ||
       !m_mouse.SetRegion( 0, 0,
                           m_screen.GetWidth() - 1,
                           m_screen.GetHeight() - 1 )
    )
  {
    return false;
  }

  m_colWidth = m_screen.GetWidth() / m_console.GetCols();
  m_rowHeight = m_screen.GetHeight() / m_console.GetRows();
  screenToConsole( m_mouse.GetX(), m_mouse.GetY(), m_col, m_row );

  m_clipboardSize = m_console.GetCols() * m_console.GetRows();
  m_clipboardBuf = new char[ m_clipboardSize + 1 ];
  if ( m_clipboardBuf == NULL )
  {
    DBG(( DBG_PREFIX "Failed to allocate memory for clipboard, size=%d\n",
          m_clipboardSize + 1 ));
    return false;
  }

  // Set the safety net
  m_clipboardBuf[ m_clipboardSize ] = 0;

  // Start from Cursor state
  changeState( &m_cursorState );

  return true;
}
//-----------------------------------------------------------------------------
bool PspMouseDaemon::Run()
{
  while ( !m_currentState->isFailed() )
  {
    if ( !m_mouse.Poll() )
    {
      sleep( c_failureDelay );
      continue;
    }

    changeState(
        m_currentState->processMouse( m_mouse.GetLeft(),
                                      m_mouse.GetMid(),
                                      m_mouse.GetRight(),
                                      m_mouse.GetX(),
                                      m_mouse.GetY() )
      );
  }

  DBG(( DBG_PREFIX "Mouse daemon terminates\n" ));
  return true;
}
//-----------------------------------------------------------------------------
void PspMouseDaemon::changeState(BaseState * newState_)
{
  while ( m_currentState != newState_ )
  {
    if ( m_currentState )
      m_currentState->exitState();
    m_currentState = newState_;
    newState_ = m_currentState->enterState();
  }
}
//-----------------------------------------------------------------------------
void PspMouseDaemon::screenToConsole(int x_, int y_, int & col_, int & row_)
{
  col_ = x_ / m_colWidth;
  row_ = y_ / m_rowHeight;
}
//-----------------------------------------------------------------------------
void PspMouseDaemon::consoleToLinear(int col_, int row_, int & pos_)
{
  int cols = m_console.GetCols();
  int rows = m_console.GetRows();

  if ( col_ < 0 )
    col_ = 0;
  else if ( col_ >= cols )
    col_ = cols - 1;

  if ( row_ < 0 )
    row_ = 0;
  else if ( row_ >= rows )
    row_ = rows - 1;

  pos_ = row_ * cols + col_;
  if ( pos_ < 0 )
    pos_ = 0;
  else if ( pos_ >= m_clipboardSize )
    pos_ = m_clipboardSize - 1;
}
//-----------------------------------------------------------------------------
void PspMouseDaemon::linearToConsole(int pos_, int & col_, int & row_)
{
  if ( pos_ < 0 )
    pos_ = 0;
  else if ( pos_ >= m_clipboardSize )
    pos_ = m_clipboardSize - 1;

  col_ = pos_ % m_console.GetCols();
  row_ = pos_ / m_console.GetCols();
}
//-----------------------------------------------------------------------------
bool PspMouseDaemon::draw
(
  int col_,
  int row_,
  bool cursor_,
  bool highlight_
)
{
  int x = col_ * m_colWidth;
  int y = row_ * m_rowHeight;
  unsigned int * start = m_screen.GetAddress() +
                         x +
                         y * m_screen.GetVirtualWidth();

  unsigned int code;
  if ( cursor_ && highlight_ )
  {
    if ( ( *start & c_cursorMask ) &&
         ( *start & c_highlightMask ) )
    {
      // Cursor and highlight are overlaying each other
      code = 0;
    }
    else if ( *start & c_cursorMask )
    {
      // Only Cursor is drawn
      code = c_highlightColor;
    }
    else if ( *start & c_highlightMask )
    {
      // Only highlight is drawn
      code = c_cursorColor;
    }
    else
    {
      // Nothing is there
      code = c_cursorColor ^ c_highlightColor;
    }
  }
  else if ( cursor_ )
  {
    if ( *start & c_cursorMask )
    {
      // Cursor is there
      code = 0;
    }
    else
    {
      code = c_cursorColor;
    }
  }
  else
  {
    if ( *start & c_highlightMask )
    {
      // Highlight is there
      code = 0;
    }
    else
    {
      code = c_highlightColor;
    }
  }

  if ( code != 0 )
    return m_screen.Xor( x, y, m_colWidth, m_rowHeight, code );

  return true;
}
//-----------------------------------------------------------------------------
bool PspMouseDaemon::clear
(
  int col_,
  int row_,
  bool cursor_,
  bool highlight_
)
{
  int x = col_ * m_colWidth;
  int y = row_ * m_rowHeight;
  unsigned int * start = m_screen.GetAddress() +
                         x +
                         y * m_screen.GetVirtualWidth();

  unsigned int code;
  if ( cursor_ && highlight_ )
  {
    if ( ( *start & c_cursorMask ) &&
         ( *start & c_highlightMask ) )
    {
      // Cursor and highlight are overlaying each other
      code = c_cursorColor ^ c_highlightColor;
    }
    else if ( *start & c_cursorMask )
    {
      // Only Cursor is drawn
      code = c_cursorColor;
    }
    else if ( *start & c_highlightMask )
    {
      // Only highlight is drawn
      code = c_highlightColor;
    }
    else
    {
      // Nothing is there
      code = 0;
    }
  }
  else if ( cursor_ )
  {
    if ( *start & c_cursorMask )
    {
      // Cursor is there
      code = c_cursorColor;
    }
    else
    {
      code = 0;
    }
  }
  else
  {
    if ( *start & c_highlightMask )
    {
      // Highlight is there
      code = c_highlightColor;
    }
    else
    {
      code = 0;
    }
  }

  if ( code != 0 )
    return m_screen.Xor( x, y, m_colWidth, m_rowHeight, code );

  return true;
}
//-----------------------------------------------------------------------------
bool PspMouseDaemon::sync()
{
  return m_screen.Sync();
}
//-----------------------------------------------------------------------------
bool PspMouseDaemon::copyCb(int begin_, int end_)
{
  if ( m_clipboardBuf == NULL )
  {
    DBG(( DBG_PREFIX "Invalid clipboard for copy\n" ));
    return false;
  }

  if ( begin_ > end_ )
  {
    int temp = begin_;
    begin_ = end_;
    end_ = temp;
  }

  if ( begin_ < 0 )
    begin_ = 0;

  if ( end_ >= m_clipboardSize )
    end_ = m_clipboardSize - 1;

  const unsigned int size = (unsigned int)( end_ - begin_ + 1 );
  unsigned int bytesRead = 0;

  if ( !m_console.Seek( (unsigned int)begin_ ) ||
       !m_console.Read( m_clipboardBuf, size, bytesRead ) )
  {
    return false;
  }

  if ( bytesRead != size )
  {
    DBG(( DBG_PREFIX "Only %d out of %d bytes are copied\n", bytesRead, size ));
  }

  m_clipboardBuf[ bytesRead ] = 0;
  return true;
}
//-----------------------------------------------------------------------------
bool PspMouseDaemon::pasteCb()
{
  if ( m_clipboardBuf == NULL )
  {
    DBG(( DBG_PREFIX "Invalid clipboard for paste\n" ));
    return false;
  }

  return m_console.Paste( m_clipboardBuf );
}
//-----------------------------------------------------------------------------
bool PspMouseDaemon::clearCb()
{
  if ( m_clipboardBuf == NULL )
  {
    DBG(( DBG_PREFIX "Invalid clipboard for clear\n" ));
    return false;
  }

  m_clipboardBuf[ 0 ] = 0;
  return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
