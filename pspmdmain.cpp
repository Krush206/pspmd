/*-----------------------------------------------------------------------------
 * Text console Mouse Daemon for uClinux on PSP
 * Created by Jackson Mo, Jan 29, 2008
 *---------------------------------------------------------------------------*/
#include "pspmd.h"
#include <stdio.h>
#include <string.h>


//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
static void showVersion();
static void showHelp();


//-----------------------------------------------------------------------------
// Implementations
//-----------------------------------------------------------------------------
int main(int argc_, char * argv_[])
{
  if ( argc_ < 2 || strcmp( argv_[ 1 ], "--help" ) == 0 )
  {
    showHelp();
    return 0;
  }
  else if ( strcmp( argv_[ 1 ], "--version" ) == 0 )
  {
    showVersion();
    return 0;
  }

  PspMouseDaemon dm;

  if ( !dm.Initialize() )
  {
    DBG(( DBG_PREFIX "Failed to launch the daemon because of a previous error\n" ));
    return -1;
  }

  // Then just run it!
  (void)dm.Run();

  return 0;
}
//-----------------------------------------------------------------------------
static void showVersion()
{
  printf( "PSP Mouse Daemon v0.1 for PSP, created by Jackson Mo\n" );
}
//-----------------------------------------------------------------------------
static void showHelp()
{
  showVersion();
  printf( "Usage: pspmd [options]\n"
          "  --help     Show this help\n"
          "  --version  Show version info\n"
          "  -s         Silent mode\n" );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
