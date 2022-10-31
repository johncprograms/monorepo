// Copyright (c) John A. Carlos Jr., all rights reserved.

#ifdef MAC

  #define _countof( array )   ( sizeof( array ) / sizeof( ( array )[0] ) )

  #define __fallthrough   /*nothing*/
  
#endif
