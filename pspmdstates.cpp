/*-----------------------------------------------------------------------------
 * Text console Mouse Daemon for uClinux on PSP
 * Created by Jackson Mo, Jan 29, 2008
 *---------------------------------------------------------------------------*/
#include "pspmd.h"
#include <stdio.h>


//-----------------------------------------------------------------------------
// class PspMouseDaemon::BaseState
//-----------------------------------------------------------------------------
PspMouseDaemon::BaseState::BaseState(PspMouseDaemon & md_)
  : m_md( md_ )
{
}
//-----------------------------------------------------------------------------
PspMouseDaemon::BaseState * 
PspMouseDaemon::BaseState::enterState()
{
  // Do nothing by default
  return this;
}
//-----------------------------------------------------------------------------
void PspMouseDaemon::BaseState::exitState()
{
  // Do nothing by default
}
//-----------------------------------------------------------------------------
PspMouseDaemon::BaseState *
PspMouseDaemon::BaseState::processMouse
(
  bool left_,
  bool mid_,
  bool right_,
  int x_,
  int y_
)
{
  // Do nothing by default
  return this;
}


//-----------------------------------------------------------------------------
// class PspMouseDaemon::FailedState
//-----------------------------------------------------------------------------
PspMouseDaemon::FailedState::FailedState(PspMouseDaemon & md_)
  : BaseState( md_ )
{
}
//-----------------------------------------------------------------------------
PspMouseDaemon::BaseState * 
PspMouseDaemon::FailedState::enterState()
{
  DBG(( DBG_PREFIX "Enter failure state!\n" ));
  return this;
}


//-----------------------------------------------------------------------------
// class PspMouseDaemon::CursorState
//-----------------------------------------------------------------------------
PspMouseDaemon::CursorState::CursorState(PspMouseDaemon & md_)
  : BaseState( md_ )
{
}
//-----------------------------------------------------------------------------
PspMouseDaemon::BaseState * 
PspMouseDaemon::CursorState::enterState()
{
  // Draw the cursor
  (void)m_md.draw( m_md.m_col, m_md.m_row, true, false );
  (void)m_md.sync();

  return this;
}
//-----------------------------------------------------------------------------
PspMouseDaemon::BaseState *
PspMouseDaemon::CursorState::processMouse
(
  bool left_,
  bool mid_,
  bool right_,
  int x_,
  int y_
)
{
  // Move the cursor
  int col, row;
  m_md.screenToConsole( x_, y_, col, row );

  if ( col != m_md.m_col || row != m_md.m_row )
  {
    (void)m_md.clear( m_md.m_col, m_md.m_row, true, false );
    (void)m_md.draw( col, row, true, false );
    (void)m_md.sync();

    m_md.m_col = col;
    m_md.m_row = row;
  }

  if ( left_ )
  {
    return &m_md.m_highlightState;
  }

  if ( right_ )
  {
    (void)m_md.pasteCb();
  }

  return this;
}


//-----------------------------------------------------------------------------
// class PspMouseDaemon::HighlightState
//-----------------------------------------------------------------------------
PspMouseDaemon::HighlightState::HighlightState(PspMouseDaemon & md_)
  : BaseState( md_ ),
    m_begin( 0 ),
    m_end( 0 ),
    m_empty( true )
{
}
//-----------------------------------------------------------------------------
PspMouseDaemon::BaseState * 
PspMouseDaemon::HighlightState::enterState()
{
  // Clear the previous highlight area if there is any
  if ( !m_empty )
    (void)clearHl( m_begin, m_end );

  // Record the beginning position
  m_md.consoleToLinear( m_md.m_col, m_md.m_row, m_begin );
  m_end = m_begin;
  m_empty = true;

  return this;
}
//-----------------------------------------------------------------------------
void PspMouseDaemon::HighlightState::exitState()
{
  if ( m_empty )
    (void)m_md.clearCb();
  else
    (void)m_md.copyCb( m_begin, m_end );
}
//-----------------------------------------------------------------------------
PspMouseDaemon::BaseState *
PspMouseDaemon::HighlightState::processMouse
(
  bool left_,
  bool mid_,
  bool right_,
  int x_,
  int y_
)
{
  int col, row;
  m_md.screenToConsole( x_, y_, col, row );

  if ( col != m_md.m_col || row != m_md.m_row )
  {
    // Since the cursor has been moved, the content to-be-copied is not empty
    if ( m_empty )
    {
      m_empty = false;
      // Clear the cursor
      (void)m_md.clear( m_md.m_col, m_md.m_row, true, false );
    }

    m_md.m_col = col;
    m_md.m_row = row;

    int newEnd;
    m_md.consoleToLinear( col, row, newEnd );

    if ( m_begin <= newEnd && newEnd < m_end )
      (void)clearHl( newEnd + 1, m_end );
    
    else if ( newEnd < m_begin && m_begin < m_end )
      (void)clearHl( m_begin + 1, m_end );

    else if ( m_end < newEnd && newEnd <= m_begin )
      (void)clearHl( m_end, newEnd - 1 );
    
    else if ( m_end < m_begin && m_begin < newEnd )
      (void)clearHl( m_end, m_begin - 1 );

    m_end = newEnd;
    (void)drawHl( m_begin, m_end );
    (void)m_md.sync();
  }

  if ( !left_ )
  {
    // If left button is released, go back to cursor mode
    return &m_md.m_cursorState;
  }

  return this;
}
//-----------------------------------------------------------------------------
bool PspMouseDaemon::HighlightState::drawHl(int begin_, int end_)
{
  int from = ( begin_ < end_ ) ? begin_ : end_;
  int to   = ( begin_ < end_ ) ? end_ : begin_;

  for ( int i = from; i <= to; i++ )
  {
    int col, row;
    m_md.linearToConsole( i, col, row );

    if ( !m_md.draw( col, row, false, true ) )
      return false;
  }

  return true;
}
//-----------------------------------------------------------------------------
bool PspMouseDaemon::HighlightState::clearHl(int begin_, int end_)
{
  int from = ( begin_ < end_ ) ? begin_ : end_;
  int to   = ( begin_ < end_ ) ? end_ : begin_;

  for ( int i = from; i <= to; i++ )
  {
    int col, row;
    m_md.linearToConsole( i, col, row );

    if ( !m_md.clear( col, row, false, true ) )
      return false;
  }

  return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
