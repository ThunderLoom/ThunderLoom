#include <Windows.h>
#include <iostream>
#include <sstream>

 #define DBOUT( s )            \
{                             \
   std::wostringstream os_;    \
   os_ << s << L"\n";            \
   OutputDebugStringW( os_.str().c_str() );  \
}