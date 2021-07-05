#ifndef PSPMD_STATES_H
#error "This header is embedded inside pspmd.h. DO NOT use it directly."
#endif

//-----------------------------------------------------------------------------
// class PspMouseDaemon::BaseState
//-----------------------------------------------------------------------------
class BaseState
{
public:
  BaseState(PspMouseDaemon & md_);
  virtual ~BaseState() { }

  virtual BaseState * enterState();
  virtual void        exitState();
  virtual BaseState * processMouse(bool left_, bool mid_, bool right_, int x_, int y_);
  virtual bool        isFailed()  { return false; }

protected:
  PspMouseDaemon & m_md;

private:
  // Not implemented
  BaseState();
  BaseState(const BaseState &);
  BaseState & operator = (const BaseState &);
};


//-----------------------------------------------------------------------------
// class PspMouseDaemon::FailedState
//-----------------------------------------------------------------------------
class FailedState : public BaseState
{
public:
  FailedState(PspMouseDaemon & md_);

  virtual BaseState * enterState();
  virtual bool        isFailed()  { return true; }

protected:
private:
  // Not implemented
  FailedState();
  FailedState(const FailedState &);
  FailedState & operator = (const FailedState &);
};


//-----------------------------------------------------------------------------
// class PspMouseDaemon::CursorState
//-----------------------------------------------------------------------------
class CursorState : public BaseState
{
public:
  CursorState(PspMouseDaemon & md_);

  virtual BaseState * enterState();
  virtual BaseState * processMouse(bool left_, bool mid_, bool right_, int x_, int y_);

protected:

private:
  // Not implemented
  CursorState();
  CursorState(const CursorState &);
  CursorState & operator = (const CursorState &);
};


//-----------------------------------------------------------------------------
// class PspMouseDaemon::HighlightState
//-----------------------------------------------------------------------------
class HighlightState : public BaseState
{
public:
  HighlightState(PspMouseDaemon & md_);

  virtual BaseState * enterState();
  virtual void        exitState();
  virtual BaseState * processMouse(bool left_, bool mid_, bool right_, int x_, int y_);

protected:
  bool drawHl(int begin_, int end_);
  bool clearHl(int begin_, int end_);

  int m_begin;
  int m_end;
  bool m_empty;

private:
  // Not implemented
  HighlightState();
  HighlightState(const HighlightState &);
  HighlightState & operator = (const HighlightState &);
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
