// Copyright (c) John A. Carlos Jr., all rights reserved.

struct
board_t
{
  u8 entries[9][9];
  bool readonly[9][9];
  bool possibles[9][9][10];
};

Inl void
Zero( board_t& board )
{
  for( u8 y = 0;  y < 9;  ++y ) {
  for( u8 x = 0;  x < 9;  ++x ) {
    board.entries[y][x] = 0;
    board.readonly[y][x] = 0;
    for( u8 entry = 0;  entry < 10;  ++entry ) {
      board.possibles[y][x][entry] = 0;
    }
  }
  }
}

Inl bool*
GetPossibles( board_t& board, u8 x, u8 y )
{
  AssertCrash( x < 9 );
  AssertCrash( y < 9 );
  return board.possibles[y][x];
}

Inl bool
GetPossible( board_t& board, u8 x, u8 y, u8 entry )
{
  AssertCrash( x < 9 );
  AssertCrash( y < 9 );
  AssertCrash( entry < 10 );
  return board.possibles[y][x][entry];
}

Inl void
SetPossible( board_t& board, u8 x, u8 y, u8 entry, bool possible )
{
  AssertCrash( x < 9 );
  AssertCrash( y < 9 );
  AssertCrash( entry < 10 );
  board.possibles[y][x][entry] = possible;
}

Inl void
SetReadonly( board_t& board, u8 x, u8 y, bool readonly )
{
  AssertCrash( x < 9 );
  AssertCrash( y < 9 );
  board.readonly[y][x] = readonly;
}

Inl u8
Read( board_t& board, u8 x, u8 y )
{
  AssertCrash( x < 9 );
  AssertCrash( y < 9 );
  return board.entries[y][x];
}

Inl bool
Write( board_t& board, u8 x, u8 y, u8 entry )
{
  AssertCrash( x < 9 );
  AssertCrash( y < 9 );
  AssertCrash( entry < 10 );
  if( board.readonly[y][x] ) {
    return 0;
  } else {
    board.entries[y][x] = entry;
    return 1;
  }
}

Inl bool
WriteAndEraseScratch( board_t& board, u8 ax, u8 ay, u8 entry )
{
  AssertCrash( ax < 9 );
  AssertCrash( ay < 9 );
  AssertCrash( entry < 10 );
  if( board.readonly[ay][ax] ) {
    return 0;

  } else {
    board.entries[ay][ax] = entry;

    for( u8 y = 0;  y < 9;  ++y ) {
      SetPossible( board, ax, y, entry, 0 );
    }
    for( u8 x = 0;  x < 9;  ++x ) {
      SetPossible( board, x, ay, entry, 0 );
    }

    u8 bx = ax / 3;
    u8 by = ay / 3;
    u8 x0 = 3 * bx;
    u8 y0 = 3 * by;
    u8 x1 = x0 + 3;
    u8 y1 = y0 + 3;
    for( u8 x = x0;  x < x1;  ++x ) {
    for( u8 y = y0;  y < y1;  ++y ) {
      SetPossible( board, x, y, entry, 0 );
    }
    }

    // reset possible to 1 where we wrote.
    SetPossible( board, ax, ay, entry, 1 );
    return 1;
  }
}



//
// Canonical sudoku solution:
//
//  1 4 7  2 5 8  3 6 9
//  2 5 8  3 6 9  1 4 7
//  3 6 9  1 4 7  2 5 8
//
//  4 7 1  5 8 2  6 9 3
//  5 8 2  6 9 3  4 7 1
//  6 9 3  4 7 1  5 8 2
//
//  7 1 4  8 2 5  9 3 6
//  8 2 5  9 3 6  7 1 4
//  9 3 6  7 1 4  8 2 5
//
Inl void
SetCanonicalSolution( board_t& board )
{
  static const u8 canonical[9][9] = {
    { 1, 4, 7,  2, 5, 8,  3, 6, 9 },
    { 2, 5, 8,  3, 6, 9,  1, 4, 7 },
    { 3, 6, 9,  1, 4, 7,  2, 5, 8 },
    { 4, 7, 1,  5, 8, 2,  6, 9, 3 },
    { 5, 8, 2,  6, 9, 3,  4, 7, 1 },
    { 6, 9, 3,  4, 7, 1,  5, 8, 2 },
    { 7, 1, 4,  8, 2, 5,  9, 3, 6 },
    { 8, 2, 5,  9, 3, 6,  7, 1, 4 },
    { 9, 3, 6,  7, 1, 4,  8, 2, 5 },
  };
  for( u8 y = 0;  y < 9;  ++y ) {
    Memmove( board.entries[y], canonical[y], 9 );
  }

  for( u8 y = 0;  y < 9;  ++y ) {
  for( u8 x = 0;  x < 9;  ++x ) {
    SetPossible( board, x, y, Read( board, x, y ), 1 );
  }
  }
}


Inl void
SetEmptySolution( board_t& board )
{
  for( u8 y = 0;  y < 9;  ++y ) {
  for( u8 x = 0;  x < 9;  ++x ) {
    Write( board, x, y, 0 );
    for( u8 entry = 1;  entry < 10;  ++entry ) {
      SetPossible( board, x, y, entry, 1 );
    }
  }
  }
}



struct
singlecheck_t
{
  bool present[10];
  bool has_nonzero_dupes;
};

Inl void
Add( singlecheck_t& check, u8 entry )
{
  AssertCrash( entry < 10 );
  if( entry  &&  check.present[entry] ) {
    check.has_nonzero_dupes = 1;
  } else {
    check.present[entry] = 1;
  }
}

bool
ValidAndFilled( singlecheck_t& check )
{
  bool r = 1;

  // All squares must be filled.
  r &= !check.present[0];

  // No duplicates.
  r &= !check.has_nonzero_dupes;

  // All numbers must be present.
  for( u8 i = 1;  i < 10;  ++i ) {
    r &= check.present[i];
  }
  return r;
}

bool
Valid( singlecheck_t& check )
{
  bool r = 1;
  // No duplicates.
  r &= !check.has_nonzero_dupes;
  return r;
}




Inl bool
ValidAndFilledRow( board_t& board, u8 y )
{
  singlecheck_t check = {};
  for( u8 x = 0;  x < 9;  ++x ) {
    Add( check, Read( board, x, y ) );
  }
  return ValidAndFilled( check );
}
Inl bool
ValidRow( board_t& board, u8 y )
{
  singlecheck_t check = {};
  for( u8 x = 0;  x < 9;  ++x ) {
    Add( check, Read( board, x, y ) );
  }
  return Valid( check );
}

Inl bool
ValidAndFilledCol( board_t& board, u8 x )
{
  singlecheck_t check = {};
  for( u8 y = 0;  y < 9;  ++y ) {
    Add( check, Read( board, x, y ) );
  }
  return ValidAndFilled( check );
}
Inl bool
ValidCol( board_t& board, u8 x )
{
  singlecheck_t check = {};
  for( u8 y = 0;  y < 9;  ++y ) {
    Add( check, Read( board, x, y ) );
  }
  return Valid( check );
}

Inl bool
ValidAndFilledBigSq( board_t& board, u8 bx, u8 by )
{
  u8 x0 = 3 * bx;
  u8 y0 = 3 * by;
  u8 x1 = x0 + 3;
  u8 y1 = y0 + 3;
  singlecheck_t check = {};
  for( u8 x = x0;  x < x1;  ++x ) {
  for( u8 y = y0;  y < y1;  ++y ) {
    Add( check, Read( board, x, y ) );
  }
  }
  return ValidAndFilled( check );
}
Inl bool
ValidBigSq( board_t& board, u8 bx, u8 by )
{
  u8 x0 = 3 * bx;
  u8 y0 = 3 * by;
  u8 x1 = x0 + 3;
  u8 y1 = y0 + 3;
  singlecheck_t check = {};
  for( u8 x = x0;  x < x1;  ++x ) {
  for( u8 y = y0;  y < y1;  ++y ) {
    Add( check, Read( board, x, y ) );
  }
  }
  return Valid( check );
}



bool
ValidAndFilled( board_t& board )
{
  bool r = 1;
  for( u8 y = 0;  y < 9;  ++y ) {
    r &= ValidAndFilledRow( board, y );
  }
  for( u8 x = 0;  x < 9;  ++x ) {
    r &= ValidAndFilledCol( board, x );
  }
  for( u8 bx = 0;  bx < 3;  ++bx ) {
  for( u8 by = 0;  by < 3;  ++by ) {
    r &= ValidAndFilledBigSq( board, bx, by );
  }
  }
  return r;
}
bool
Valid( board_t& board )
{
  bool r = 1;
  for( u8 y = 0;  y < 9;  ++y ) {
    r &= ValidRow( board, y );
  }
  for( u8 x = 0;  x < 9;  ++x ) {
    r &= ValidCol( board, x );
  }
  for( u8 bx = 0;  bx < 3;  ++bx ) {
  for( u8 by = 0;  by < 3;  ++by ) {
    r &= ValidBigSq( board, bx, by );
  }
  }
  return r;
}



Inl void
RenderBoard(
  client_t& client,
  board_t& board,
  vec2<f32> board_pos,
  font_t& font,
  font_t& smallfont,
  vec2<f32> window_size,
  vec2<f32> square_size,
  vec2<f32> square_text_offset,
  vec2<f32> smallsquare_size,
  vec2<f32> smallsquare_text_offset,
  vec2<f32> bigsquare_size,
  vec2<f32> board_size
  )
{
  static const auto rgba_text = _vec4<f32>( 1, 1, 1, 1 );
  static const auto rgba_bigsquare_lines = _vec4<f32>( 0.5f, 0.5f, 0.5f, 1.0f );
  static const auto rgba_square_lines = _vec4<f32>( 0.25f, 0.25f, 0.25f, 1.0f );

  Fori( u8, y, 0, 9 ) {
  Fori( u8, x, 0, 9 ) {
    auto entry = Read( board, x, y );

    auto ipos = _vec2<f32>( x, y );
    auto clip0 = board_pos + ipos * square_size;
    auto clip1 = clip0 + square_size;
    auto textpos = clip0 + square_text_offset;

    static const void* entrytexts[] = {
      "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    };

    if( entry ) {
      ptext_t text;
      text.font_id = font.id;
      text.spaces_per_tab = 2;
      text.z = 0;
      text.cliprect[0] = clip0;
      text.cliprect[1] = clip1;
      text.color = rgba_text;
      text.text_len = 1;
      text.text = Str( entrytexts[entry] );
      text.pos = textpos;
      Glw::RenderText( client, text );
    } else {
      auto possibles = GetPossibles( board, x, y );

      Fori( u8, ientry, 1, 10 ) {
        auto entry_possible = possibles[ientry];
        if( entry_possible ) {
          u8 entry_row = ( ientry - 1 ) / 3;
          u8 entry_col = ( ientry - 1 ) % 3;

          auto ientrypos = _vec2<f32>( entry_col, entry_row );
          auto entry_clip0 = clip0 + ientrypos * smallsquare_size;
          auto entry_clip1 = entry_clip0 + smallsquare_size;
          auto entry_pos = entry_clip0 + smallsquare_text_offset;

          ptext_t text;
          text.font_id = smallfont.id;
          text.spaces_per_tab = 2;
          text.z = 0;
          text.cliprect[0] = entry_clip0;
          text.cliprect[1] = entry_clip1;
          text.color = rgba_text;
          text.text_len = 1;
          text.text = Str( entrytexts[ientry] );
          text.pos = entry_pos;
          Glw::RenderText( client, text );
        }
      }
    }
  }
  }

  for( u8 i = 0;  i < 9;  ++i ) {
    if( i % 3 == 0 ) {
      continue;
    }
    auto x = board_pos.x + Cast( f32, i ) * square_size.x;
    auto y0 = board_pos.y + 0.0f;
    auto y1 = board_pos.y + board_size.y;

    line_t line;
    line.color = rgba_square_lines;
    line.verts[0] = _vec2( x, y0 );
    line.verts[1] = _vec2( x, y1 );
    line.z = 0;
    line.cliprect[0] = board_pos;
    line.cliprect[1] = window_size;
    line.width = 1;
    Glw::RenderLine( client, line );
  }
  for( u8 j = 0;  j < 9;  ++j ) {
    if( j % 3 == 0 ) {
      continue;
    }
    auto y = board_pos.y + Cast( f32, j ) * square_size.y;
    auto x0 = board_pos.x + 0.0f;
    auto x1 = board_pos.x + board_size.x;

    line_t line;
    line.color = rgba_square_lines;
    line.verts[0] = _vec2( x0, y );
    line.verts[1] = _vec2( x1, y );
    line.z = 0;
    line.cliprect[0] = board_pos;
    line.cliprect[1] = window_size;
    line.width = 1;
    Glw::RenderLine( client, line );
  }

  for( u8 bx = 0;  bx <= 3;  ++bx ) {
    auto x = board_pos.x + Cast( f32, bx ) * bigsquare_size.x;
    auto y0 = board_pos.y + 0.0f;
    auto y1 = board_pos.y + board_size.y;

    line_t line;
    line.color = rgba_bigsquare_lines;
    line.verts[0] = _vec2( x, y0 );
    line.verts[1] = _vec2( x, y1 );
    line.z = 0;
    line.cliprect[0] = board_pos;
    line.cliprect[1] = window_size;
    line.width = 1;
    Glw::RenderLine( client, line );
  }
  for( u8 by = 0;  by <= 3;  ++by ) {
    auto y = board_pos.y + Cast( f32, by ) * bigsquare_size.y;
    auto x0 = board_pos.x + 0.0f;
    auto x1 = board_pos.x + board_size.x;

    line_t line;
    line.color = rgba_bigsquare_lines;
    line.verts[0] = _vec2( x0, y );
    line.verts[1] = _vec2( x1, y );
    line.z = 0;
    line.cliprect[0] = board_pos;
    line.cliprect[1] = window_size;
    line.width = 1;
    Glw::RenderLine( client, line );
  }
}


Inl void
MapMouseToBoard(
  s32 mx,
  s32 my,
  vec2<f32> board_pos,
  vec2<f32> square_size,
  bool& over_board,
  u8& x,
  u8& y )
{
  auto mpos = _vec2( Cast( f32, mx ), Cast( f32, my ) );
  auto pos = ( mpos - board_pos ) / square_size;
  auto tx = Floor_s32_from_f32( pos.x );
  auto ty = Floor_s32_from_f32( pos.y );
  bool validx = ( 0 <= tx )  &  ( tx < 9 );
  bool validy = ( 0 <= ty )  &  ( ty < 9 );
  if( validx  &  validy ) {
    over_board = 1;
    x = Cast( u8, tx );
    y = Cast( u8, ty );
  } else {
    over_board = 0;
    x = 0;
    y = 0;
  }
}


