// Copyright (c) John A. Carlos Jr., all rights reserved.


static constexpr idx_t c_fspath_len = 384;

// Holds metadata for a file OR a directory in a filesystem.
typedef embeddedarray_t<u8, c_fspath_len> fsobj_t;

Inl fsobj_t
_fsobj( void* cstr )
{
  fsobj_t r;
  r.len = CsLen( Cast( u8*, cstr ) );
  Memmove( r.mem, cstr, r.len );
  return r;
}


Inl u64
_GetFileTime( WIN32_FIND_DATA& f )
{
  u64 time = Pack( f.ftLastWriteTime.dwHighDateTime, f.ftLastWriteTime.dwLowDateTime );
  return time;
}

Inl u64
_GetFileSize( WIN32_FIND_DATA& f )
{
  u64 size;
  size = Cast( u64, f.nFileSizeHigh ) << 32;
  size |= Cast( u64, f.nFileSizeLow );
  return size;
}

Inl bool
_IsRegularDir( WIN32_FIND_DATA& f )
{
  auto len = CsLen( Str( f.cFileName ) );
  bool r =
    ( f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
    !CsEquals( Str( f.cFileName ), len, Str( "." ), 1, 1 ) &&
    !CsEquals( Str( f.cFileName ), len, Str( ".." ), 2, 1 );
  return r;
}

Inl bool
_IsFile( WIN32_FIND_DATA& f )
{
  return !( f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
}



static bool
_ObjectExists( u8* name )
{
  WIN32_FIND_DATA f;
  HANDLE h = FindFirstFile( Cast( char*, name ), &f );
  if( INVALID_HANDLE_VALUE == h ) {
    return 0;
  }
  AssertWarn( FindClose( h ) );
  return 1;
}

static bool
_RecycleFsObj( u8* name, idx_t len )
{
  AssertCrash( name[len] == 0 );

  u8 fullpath[c_fspath_len + 2];
  u8* name_only = 0;
  DWORD res = GetFullPathName( Cast( char*, name ), sizeof( fullpath ), Cast( char*, fullpath ), Cast( char**, &name_only ) );
  if( !res ) {
    return 0;
  }

  idx_t fullpath_len = CsLen( fullpath );
  fullpath[fullpath_len + 0] = 0;
  fullpath[fullpath_len + 1] = 0; // double 0-term for SHFILEOPSTRUCT.

  SHFILEOPSTRUCT recycle = {};
  recycle.hwnd = 0;
  recycle.wFunc = FO_DELETE;
  recycle.pFrom = Cast( char*, fullpath );
  recycle.pTo = "\0\0";
  recycle.fFlags = FOF_ALLOWUNDO;
  recycle.lpszProgressTitle = "Sending fsobj to recycle bin.";

  int operr = SHFileOperation( &recycle );
  if( operr ) {
    return 0;
  }

  if( recycle.fAnyOperationsAborted ) {
    return 0;
  }

  return 1;
}



// utility to standardize dir/file pathnames.

void
_FixupDir( u8* dir, idx_t* dir_len )
{
  idx_t len = *dir_len;
  if( len ) {
    // leave leading backslashes in place, since windows requires network shares to start with two of them.
    idx_t start = 0;
    For( i, 0, len ) {
      if( dir[i] != '\\' ) {
        break;
      }
      ++start;
    }
    CsReplace( dir + start, len - start, '\\', '/' );
    if( dir[len - 1] == '/'  ||  dir[len - 1] == '\\' ) {
      dir[len - 1] = 0;
      len -= 1;
    }
  }
  *dir_len = len;
}

void
_FixupFile( u8* file, idx_t* file_len )
{
  CsReplace( file, *file_len, '\\', '/' );
}

Inl fsobj_t
_StandardFilename( u8* name, idx_t len )
{
  fsobj_t file;
  file.len = 0;
  Memmove( AddBack( file, len ), name, len );
  Memmove( AddBack( file ), "\0", 1 );
  RemBack( file );
  _FixupFile( file.mem, &file.len );
  return file;
}

Inl fsobj_t
_StandardDirname( u8* name, idx_t len )
{
  fsobj_t dir;
  dir.len = 0;
  Memmove( AddBack( dir, len ), name, len );
  Memmove( AddBack( dir ), "\0", 1 );
  RemBack( dir );
  _FixupDir( dir.mem, &dir.len );
  return dir;
}

Inl slice_t
FileExtension( u8* name, idx_t len )
{
  slice_t r;
  auto last_dot = CsScanL( name, len, '.' );
  if( last_dot ) {
    auto fileext = last_dot + 1;
    r.mem = fileext;
    r.len = name + len - fileext;
  }
  else {
    r = {};
  }
  return r;
}

Inl slice_t
FileNameAndExt( u8* name, idx_t len )
{
  slice_t r;
  auto last_slash = CsScanL( name, len, '/' );
  if( last_slash ) {
    auto name_start = last_slash + 1;
    r.mem = name_start;
    r.len = name + len - name_start;
  }
  else {
    r.mem = name;
    r.len = len;
  }
  return r;
}

Inl slice_t
FileNameOnly( u8* name, idx_t len )
{
  auto name_and_ext = FileNameAndExt( name, len );
  auto ext = FileExtension( name, len );
  auto r = name_and_ext;
  if( ext.len ) {
    AssertCrash( r.len >= ext.len + 1 );
    r.len -= ext.len + 1;
  }
  return r;
}

Inl void
FsGetExe( u8* dst, idx_t dst_len, idx_t* exe_len )
{
  AssertCrash( dst_len <= MAX_s32 );
  DWORD r = GetModuleFileNameA( 0, Cast( char*, dst ), Cast( s32, dst_len ) );
  AssertWarn( r );
  *exe_len = CsLen( dst );
  _FixupFile( dst, exe_len );
}







void
FsGetCwd( u8* dst, idx_t dst_len, idx_t* cwd_len )
{
  AssertCrash( dst_len <= UINT_MAX );
  DWORD res = GetCurrentDirectory( Cast( DWORD, dst_len ), Cast( char*, dst ) );
  AssertWarn( res );
  *cwd_len = CsLen( dst );
  _FixupDir( dst, cwd_len );
}

void
FsGetCwd( fsobj_t& obj )
{
  FsGetCwd( obj.mem, Capacity( obj ), &obj.len );
}

void
FsSetCwd( u8* cwd, idx_t cwd_len )
{
  fsobj_t dir = _StandardDirname( cwd, cwd_len );
  BOOL res = SetCurrentDirectory( Cast( char*, dir.mem ) );
  AssertWarn( res );
}

void
FsSetCwd( fsobj_t& obj )
{
  FsSetCwd( ML( obj ) );
}





Enumc( fsiter_result_t )
{
  continue_,
  stop,
};

#define FS_ITERATOR( fnname )   \
  fsiter_result_t ( fnname )( \
    u8* name, \
    idx_t len, \
    bool is_file, \
    bool readonly, \
    u64 filesize, \
    void* misc \
    )

typedef FS_ITERATOR( *pfn_fsiterator_t );

void
FsIterate(
  u8* path,
  idx_t path_len,
  bool recur,
  pfn_fsiterator_t FsIterator,
  void* misc
  )
{
  array_t<fsobj_t> searchdirs;
  Alloc( searchdirs, 32 );
  *AddBack( searchdirs ) = _StandardDirname( path, path_len );
  while( searchdirs.len ) {
    fsobj_t searchdir = searchdirs.mem[ searchdirs.len - 1 ];
    RemBack( searchdirs );
    Memmove( AddBack( searchdir, 3 ), "/*\0", 3 );
    WIN32_FIND_DATA f;
    // PERF: try FindFirstFileEx, etc. there's params to help speed things up on Win7+
    HANDLE h = FindFirstFile( Cast( char*, searchdir.mem ), &f );
    RemBack( searchdir, 2 );
    if( h == INVALID_HANDLE_VALUE ) {
      continue;
    }
    bool more_files = 1;
    while( more_files ) {
      if( _IsFile( f ) ) {
        auto filename_len = CsLen( Cast( u8*, f.cFileName ) );
        Memmove( AddBack( searchdir, filename_len ), Cast( u8*, f.cFileName ), filename_len );
        bool readonly = f.dwFileAttributes & FILE_ATTRIBUTE_READONLY;
        u64 filesize = Pack( f.nFileSizeHigh, f.nFileSizeLow );
        auto iter_result = FsIterator( ML( searchdir ), 1, readonly, filesize, misc );
        switch( iter_result ) {
          case fsiter_result_t::continue_: {
          } break;
          case fsiter_result_t::stop: {
            searchdirs.len = 0;
            goto NO_MORE_FILES;
          } break;
        }
        RemBack( searchdir, filename_len );
      } elif( _IsRegularDir( f ) ) {
        auto filename_len = CsLen( Cast( u8*, f.cFileName ) );
        Memmove( AddBack( searchdir, filename_len ), Cast( u8*, f.cFileName ), filename_len );
        auto iter_result = FsIterator( ML( searchdir ), 0, 0, 0, misc );
        switch( iter_result ) {
          case fsiter_result_t::continue_: {
          } break;
          case fsiter_result_t::stop: {
            searchdirs.len = 0;
            goto NO_MORE_FILES;
          } break;
        }
        if( recur ) {
          *AddBack( searchdirs ) = searchdir;
        }
        RemBack( searchdir, filename_len );
      }
      more_files = !!FindNextFile( h, &f );
    }
NO_MORE_FILES:
    FindClose( h );
  }
  Free( searchdirs );
}








bool
DirExists( u8* name, idx_t len )
{
  fsobj_t dir = _StandardDirname( name, len );
  DWORD attribs = GetFileAttributes( Cast( char*, dir.mem ) );
  if( attribs == INVALID_FILE_ATTRIBUTES ) {
    return 0;
  }
  return !!( attribs & FILE_ATTRIBUTE_DIRECTORY );
}


bool
DirCreate( u8* name, idx_t len )
{
  fsobj_t dir = _StandardDirname( name, len );

  u8* slash = dir.mem;
  Forever {
    slash = CsScanR( slash, dir.len, '/' );
    if( slash ) {
      *slash = 0;
    }
    if( !DirExists( ML( dir ) ) ) {
      if( !CreateDirectory( Cast( char*, dir.mem ), 0 ) ) {
        return 0;
      }
    }
    if( slash ) {
      *slash = '/';
      slash += 1;
    } else {
      break;
    }
  }
  return 1;
}


bool
FileDelete( u8* name, idx_t len )
{
  fsobj_t file = _StandardFilename( name, len );
  if( !DeleteFile( Cast( char*, file.mem ) ) ) {
    return 0;
  }
  return 1;
}





struct
dirdeletecontents_t
{
  bool* success;
  array_t<fsobj_t>* dirs;
};
Inl
FS_ITERATOR( _IterDirDeleteContents )
{
  auto context = Cast( dirdeletecontents_t*, misc );
  if( is_file ) {
    if( !FileDelete( name, len ) ) {
      *context->success = 0;
    }
  } else {
    *AddBack( *context->dirs ) = _StandardDirname( name, len );
  }
  return fsiter_result_t::continue_;
}
bool
DirDeleteContents( u8* name, idx_t len )
{
  fsobj_t dir = _StandardDirname( name, len );
  array_t<fsobj_t> dirs_to_delete;
  Alloc( dirs_to_delete, 32 );
  bool r = 1;
  dirdeletecontents_t context;
  context.dirs = &dirs_to_delete;
  context.success = &r;
  FsIterate( name, len, 1, _IterDirDeleteContents, Cast( void*, &context ) );
  ReverseFor( i, 0, dirs_to_delete.len ) {
    auto rem = dirs_to_delete.mem + i;
    r &= !!RemoveDirectory( Cast( char*, rem->mem ) );
  }
  Free( dirs_to_delete );
  return r;
}



bool
DirDelete( u8* name, idx_t len )
{
  fsobj_t dir = _StandardDirname( name, len );
  if( !DirDeleteContents( name, len ) ) {
    return 0;
  }
  if( !RemoveDirectory( Cast( char*, dir.mem ) ) ) {
    return 0;
  }
  return 1;
}

bool
DirRecycle( u8* name, idx_t len )
{
  fsobj_t dir = _StandardDirname( name, len );
  return _RecycleFsObj( ML( dir ) );
}



bool
DirCopy( u8* dst, idx_t dst_len, u8* src, idx_t src_len )
{
  ImplementCrash();
  if( CsEquals( dst, dst_len, src, src_len, 0 ) ) {
    return 1;
  }

  //array_t<fsobj_t> files;
  //Alloc( files, 16 );
  //fsFindFiles( files, src );
  //u8 name [ c_fspath_len ];
  //for( idx_t i = 0;  i < size( files );  ++i ) {
  //  CsCopy( name, dst, CsLen( dst ) );
  //  Cstr::AddBack( name, '/' );
  //  if( files[i].PathRel[0] ) {
  //    Cstr::AddBack( name, files[i].PathRel );
  //    Cstr::AddBack( name, '/' );
  //  }
  //  Cstr::AddBack( name, files[i].NameOnly );
  //  Cstr::AddBack( name, '.' );
  //  Cstr::AddBack( name, files[i].Ext );
  //  if( !FileCopy( name, files[i].NameAbs ) )
  //    goto fsDirCopy_FAIL;
  //}
  //EditKill( files );
  //return 1;

  //fsDirCopy_FAIL:
  //EditKill( files );
  return 1;
}

bool
DirCopyOverwrite( u8* dst, idx_t dst_len, u8* src, idx_t src_len )
{
  ImplementCrash();
  if( CsEquals( dst, dst_len, src, src_len, 0 ) ) {
    return 1;
  }

  //array_t<fsobj_t> files;
  //Alloc( files, 16 );
  //fsFindFiles( files, src );
  //u8 name [ c_fspath_len ];
  //for( idx_t i = 0;  i < size( files );  ++i ) {
  //  CsCopy( name, dst, CsLen( dst ) );
  //  Cstr::AddBack( name, '/' );
  //  if( files[i].PathRel[0] ) {
  //    Cstr::AddBack( name, files[i].PathRel );
  //    Cstr::AddBack( name, '/' );
  //  }
  //  Cstr::AddBack( name, files[i].NameOnly );
  //  Cstr::AddBack( name, '.' );
  //  Cstr::AddBack( name, files[i].Ext );
  //  if( !FileCopyOverwrite( name, files[i].NameAbs ) )
  //    goto fsDirCopyOverwrite_FAIL;
  //}
  //EditKill( files );
  //return 1;

  //fsDirCopyOverwrite_FAIL:
  //EditKill( files );
  return 1;
}

bool
DirMove( u8* dst, idx_t dst_len, u8* src, idx_t src_len )
{
  fsobj_t srcdir = _StandardDirname( src, src_len );
  fsobj_t dstdir = _StandardDirname( dst, dst_len );

  if( _ObjectExists( dstdir.mem ) ) {
    return 0;
  }
  bool moved = !!MoveFile( Cast( char*, srcdir.mem ), Cast( char*, dstdir.mem ) );
  return moved;
}


struct
file_t
{
  fsobj_t obj;
  void* loaded; // stores file handle.
  u64 size;
  u64 time_create;
  u64 time_lastaccess;
  u64 time_lastwrite;
  bool readonly;
};



bool
_EnsureDstDirectory( fsobj_t& dstfile )
{
  u8* last_slash = CsScanL( ML( dstfile ), '/' );
  if( !last_slash ) {
    return 0;
  }
  fsobj_t dst_dir = {};
  CsCopy( dst_dir.mem, dstfile.mem, last_slash );
  dst_dir.len = CsLen( dst_dir.mem );
  if( !DirExists( ML( dst_dir ) ) ) {
    if( !DirCreate( ML( dst_dir ) ) ) {
      return 0;
    }
  }
  return 1;
}

#define _GetHandle( file ) \
  Cast( HANDLE, file.loaded )

#define _u64_from_FILETIME( ft ) \
  Pack( ft.dwHighDateTime, ft.dwLowDateTime )

Inl void
_PopulateMetadata( file_t& file )
{
  BY_HANDLE_FILE_INFORMATION fileinfo = {};
  AssertWarn( GetFileInformationByHandle( _GetHandle( file ), &fileinfo ) );
  file.size = Pack( fileinfo.nFileSizeHigh, fileinfo.nFileSizeLow );
  file.time_create     = _u64_from_FILETIME( fileinfo.ftCreationTime   );
  file.time_lastaccess = _u64_from_FILETIME( fileinfo.ftLastAccessTime );
  file.time_lastwrite  = _u64_from_FILETIME( fileinfo.ftLastWriteTime  );
  file.readonly = fileinfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY;
}

void
_SetFilePtr( file_t& file, u64 file_offset )
{
  BOOL res = SetFilePointerEx(
    _GetHandle( file ),
    *Cast( LARGE_INTEGER*, &file_offset ),
    0,
    FILE_BEGIN
    );
  AssertWarn( res );
}


Enumc( fileop_t )
{
  R,
  W,
  RW,
};

static const bool g_hasWrite[] = {
  0,
  1,
  1,
};

#define _HasWrite( op ) \
  g_hasWrite[Cast( enum_t, op )]

static const DWORD g_accessBits[] = {
  GENERIC_READ,
  GENERIC_WRITE,
  GENERIC_READ | GENERIC_WRITE,
};

#define _GetAccessBits( access ) \
  g_accessBits[Cast( enum_t, access )]

static const DWORD g_shareBits[] = {
  FILE_SHARE_READ,
  FILE_SHARE_WRITE,
  FILE_SHARE_READ | FILE_SHARE_WRITE,
};

#define _GetShareBits( share ) \
  g_shareBits[Cast( enum_t, share )]

Enumc( fileopen_t )
{
  only_new,
  only_existing,
  always,
};

static const DWORD g_opentype[] = {
  CREATE_NEW,
  OPEN_EXISTING,
  OPEN_ALWAYS,
};

#define _GetOpenType( open ) \
  g_opentype[Cast( enum_t, open )]



file_t
FileOpen( u8* name, idx_t len, fileopen_t type, fileop_t access, fileop_t share )
{
  file_t file = {};
  file.obj = _StandardFilename( name, len );
  DWORD attribs = FILE_ATTRIBUTE_NORMAL;

  switch( type ) {

    case fileopen_t::only_new: {
      if( !_EnsureDstDirectory( file.obj ) ) {
        return file;
      }
    } break;

    case fileopen_t::only_existing: {
      attribs = GetFileAttributes( Cast( char*, file.obj.mem ) );
      if( _HasWrite( access ) && ( attribs & FILE_ATTRIBUTE_READONLY ) ) {
        attribs &= ~FILE_ATTRIBUTE_READONLY;
        if( !SetFileAttributes( Cast( char*, file.obj.mem ), attribs ) ) {
          return file;
        }
      }
    } break;

    case fileopen_t::always: {
      if( !_EnsureDstDirectory( file.obj ) ) {
        return file;
      }
    } break;
  }

  HANDLE h = CreateFile(
    Cast( char*, file.obj.mem ),
    _GetAccessBits( access ),
    _GetShareBits( share ),
    0,
    _GetOpenType( type ),
    attribs,
    0
    );

  if( h == INVALID_HANDLE_VALUE ) {
    return file;
  }
  file.loaded = Cast( void*, h );
  _PopulateMetadata( file );
  return file;
}


// TODO: probably return success bool
void
FileRead( file_t& file, u64 file_offset, u8* dst, u64 dst_len, u32 chunk_size )
{
  _SetFilePtr( file, file_offset );
  u64 ntoread = MIN( file.size - file_offset, dst_len );
  u32 nchunks = Cast( u32, ntoread / chunk_size );
  u32 nrem    = Cast( u32, ntoread % chunk_size );
  Fori( u32, i, 0, nchunks ) {
    DWORD nread = 0;
    BOOL r = ReadFile(
      _GetHandle( file ),
      dst,
      chunk_size,
      &nread,
      0
      );
    AssertWarn( r );
    AssertWarn( nread == chunk_size );
    dst += nread;
  }

  if( nrem ) {
    DWORD nread = 0;
    BOOL r = ReadFile(
      _GetHandle( file ),
      dst,
      nrem,
      &nread,
      0
      );
    AssertWarn( r );
    AssertWarn( nread == nrem );
    dst += nread;
  }

  // update file info.
  _PopulateMetadata( file );
}


// TODO: chunk to smaller than MAX_u32, like FileRead?
void
FileWrite( file_t& file, u64 file_offset, u8* src, u64 src_len )
{
  _SetFilePtr( file, file_offset );
  u32 nchunks = Cast( u32, src_len / MAX_u32 );
  u32 nrem    = Cast( u32, src_len % MAX_u32 );
  Fori( u32, i, 0, nchunks ) {
    DWORD nwritten = 0;
    BOOL r = WriteFile(
      _GetHandle( file ),
      src,
      MAX_u32,
      &nwritten,
      0
      );
    AssertWarn( r );
    AssertWarn( nwritten == MAX_u32 );
    src += nwritten;
  }

  DWORD nwritten = 0;
  BOOL r = WriteFile(
    _GetHandle( file ),
    src,
    nrem,
    &nwritten,
    0
    );
  AssertWarn( r );
  AssertWarn( nwritten == nrem );
  src += nwritten;

  // update file info.
  _PopulateMetadata( file );
}

void
FileWriteAppend( file_t& file, u8* src, u64 src_len )
{
  FileWrite( file, file.size, src, src_len );
}


void
FileSetEOF( file_t& file, u64 file_offset )
{
  _SetFilePtr( file, file_offset );
  AssertWarn( SetEndOfFile( _GetHandle( file ) ) );
  _PopulateMetadata( file );
}
void
FileSetEOF( file_t& file )
{
  FileSetEOF( file, file.size );
}


u64
FileTimeLastWrite( u8* name, idx_t len )
{
  fsobj_t file = _StandardFilename( name, len );
  WIN32_FILE_ATTRIBUTE_DATA metadata;
  AssertWarn( GetFileAttributesEx( Cast( char*, file.mem ), GetFileExInfoStandard, &metadata ) );
  return Pack( metadata.ftLastWriteTime.dwHighDateTime, metadata.ftLastWriteTime.dwLowDateTime );
}

void
FileFree( file_t& file )
{
  if( file.loaded ) {
    AssertWarn( CloseHandle( _GetHandle( file ) ) );
  }
  file = {};
}




bool
FileRecycle( u8* name, idx_t len )
{
  fsobj_t file = _StandardFilename( name, len );
  return _RecycleFsObj( ML( file ) );
}



bool
FileExists( u8* name, idx_t len )
{
  fsobj_t file = _StandardFilename( name, len );
  WIN32_FIND_DATA f;
  HANDLE h = FindFirstFile( Cast( char*, file.mem ), &f );
  if( INVALID_HANDLE_VALUE == h ) {
    return 0;
  }
  bool r = _IsFile( f );
  AssertWarn( FindClose( h ) );
  return r;
}




bool
FileCopy( u8* dstname, idx_t dstname_len, u8* srcname, idx_t srcname_len )
{
  fsobj_t dst = _StandardFilename( dstname, dstname_len );
  fsobj_t src = _StandardFilename( srcname, srcname_len );
  if( !FileExists( ML( src ) ) ) {
    return 0;
  }
  if( !_EnsureDstDirectory( dst ) ) {
    return 0;
  }
  bool copied = !!CopyFile( Cast( char*, src.mem ), Cast( char*, dst.mem ), 1 /* FAIL if exists */ );
  return copied;
}

bool
FileCopyOverwrite( u8* dstname, idx_t dstname_len, u8* srcname, idx_t srcname_len )
{
  fsobj_t dst = _StandardFilename( dstname, dstname_len );
  fsobj_t src = _StandardFilename( srcname, srcname_len );
  if( !_EnsureDstDirectory( dst ) ) {
    return 0;
  }
  bool copied = !!CopyFile( Cast( char*, src.mem ), Cast( char*, dst.mem ), 0 /* OVR if exists */ );
  return copied;
}

bool
FileMove( u8* dstname, idx_t dstname_len, u8* srcname, idx_t srcname_len )
{
  fsobj_t dst = _StandardFilename( dstname, dstname_len );
  fsobj_t src = _StandardFilename( srcname, srcname_len );
  if( FileExists( ML( dst ) ) ) {
    return 0;
  }
  bool moved = !!MoveFile( Cast( char*, src.mem ), Cast( char*, dst.mem ) );
  return moved;
}



#if 0

u64
GetSize( file_t& file )
{
  u64 size = 0;
  AssertWarn( GetFileSizeEx( _GetHandle( file ), Cast( LARGE_INTEGER*, &size ) ) );
  return size;
}

bool
Size( u8* name, idx_t len, u64* size )
{
  fsobj_t file = _StandardFilename( name, len );

  WIN32_FILE_ATTRIBUTE_DATA metadata;
  bool res = !!GetFileAttributesEx( Cast( char*, file.mem ), GetFileExInfoStandard, &metadata );
  u64 r = 0;
  if( res ) {
    r = Cast( u64, metadata.nFileSizeHigh ) << 32ULL;
    r |= Cast( u64, metadata.nFileSizeLow );
  }
  *size = r;
  return res;
}

#endif


struct
filemapped_t
{
  u8* mapped_mem;
  idx_t size;
  void* m; // file mapping handle.
  void* loaded; // file handle.
};

filemapped_t
FileOpenMappedExistingReadShareRead( u8* filename, idx_t filename_len )
{
  fsobj_t file = _StandardFilename( filename, filename_len );

  filemapped_t ret = {};

  HANDLE f = CreateFile(
    Cast( char*, file.mem ),
    GENERIC_READ,
    FILE_SHARE_READ,
    0,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    0
    );
  if( f == INVALID_HANDLE_VALUE ) {
    return ret;
  }

  u64 file_size;
  BOOL sized = GetFileSizeEx( f, Cast( LARGE_INTEGER*, &file_size ) );
  AssertWarn( sized );
  if( !file_size ) {
    // we can't memory-map an empty file. return a valid result, but with zero size.
    ret.loaded = Cast( void*, f );

  } else {
    HANDLE m = CreateFileMapping(
      f,
      0,
      PAGE_READONLY,
      0, 0, 0
      );
    if( !m ) {
      CloseHandle( f );
      return ret;
    }

    void* p = MapViewOfFile(
      m,
      FILE_MAP_READ,
      0, 0, 0
      );
    if( !p ) {
      CloseHandle( f );
      CloseHandle( m );
      return ret;
    }

    ret.mapped_mem = Cast( u8*, p );

    AssertCrash( file_size < MAX_idx );
    ret.size = Cast( idx_t, file_size );
    ret.m = Cast( void*, m );
    ret.loaded = Cast( void*, f );
  }

  return ret;
}

void
FileFree( filemapped_t& file )
{
  if( file.size ) {
    AssertWarn( UnmapViewOfFile( file.mapped_mem ) );
    CloseHandle( Cast( HANDLE, file.m ) );
    CloseHandle( Cast( HANDLE, file.loaded ) );
  }

  file = {};
}


string_t
FileAlloc( file_t& file )
{
  AssertCrash( file.size <= MAX_idx );
  auto ntoread = Cast( idx_t, file.size );
  if( !ntoread ) {
    return {};
  }
  string_t r;
  Alloc( r, ntoread );
  // split reads into chunks of large size; ~200MB or so.
  constant u64 c_chunk_size = 200*1000*1000;
  FileRead( file, 0, ML( r ), c_chunk_size );
  return r;
}



// TODO: redo hotloading.

// effectively an FileReadAll() call, but with additional timestamp caching so
//   that you'll only go to disk if the timestamps are different.
// 'dst' is always cleared.
// copies all of the bytes into 'dst' from the 'filename' file.
// if the file is new or has changed, 'dst' will contain updated file
//   contents, and returns 1.
// if the file has NOT changed, returns 1.
// if the file doesn't exist or the read fails, returns 0.
//bool
//FileHotload( u8* filename, idx_t filename_len, slice_t* dst );



#if 0

Inl void
_FindFiles( array_t<fsobj_t>& dst, u8* dir_base, idx_t dir_base_len )
{
  WIN32_FIND_DATA f;
  HANDLE h;

  fsobj_t file = {};
  CsCopy( file.mem, dir_base, dir_base_len );
  CsAddBack( file.mem, '/' );
  u8* file_name_rel = file.mem + CsLen( file.mem );

  fsobj_t search = {};
  CsCopy( search.mem, dir_base, dir_base_len );
  CsAddBack( search.mem, Str( "/*" ), 2 );
  h = FindFirstFile( Cast( char*, search.mem ), &f );
  if( h == INVALID_HANDLE_VALUE ) {
    return;
  }

  Forever {
    if( _IsFile( f ) ) {
      CsCopy( file_name_rel, Cast( u8*, f.cFileName ) );
      file.len = CsLen( file.mem );
      *AddBack( dst ) = file;
    }
    if( !FindNextFile( h, &f ) ) {
      break;
    }
  }
  FindClose( h );
}


Inl void
_FindDirs( array_t<fsobj_t>& dst, u8* dir_base, idx_t dir_base_len )
{
  WIN32_FIND_DATA f;
  HANDLE h;

  fsobj_t dir = {};
  CsCopy( dir.mem, dir_base, dir_base_len );
  CsAddBack( dir.mem, '/' );
  u8* dir_name_rel = dir.mem + CsLen( dir.mem );

  fsobj_t search = {};
  CsCopy( search.mem, dir_base, dir_base_len );
  CsAddBack( search.mem, Str( "/*" ), 2 );
  h = FindFirstFile( Cast( char*, search.mem ), &f );
  if( h == INVALID_HANDLE_VALUE ) {
    return;
  }

  Forever {
    if( _IsRegularDir( f ) ) {
      CsCopy( dir_name_rel, Cast( u8*, f.cFileName ) );
      dir.len = CsLen( dir.mem );
      *AddBack( dst ) = dir;
    }
    if( !FindNextFile( h, &f ) ) {
      break;
    }
  }
  FindClose( h );
}





Inl void
_FindFilesRecur( array_t<fsobj_t>& dst, u8* dir_base, idx_t dir_base_len )
{
  WIN32_FIND_DATA f;
  HANDLE h;

  array_t<fsobj_t> stack_subdirs;
  Alloc( stack_subdirs, 64 );

  fsobj_t search = {};
  CsCopy( search.mem, dir_base, dir_base_len );
  CsAddBack( search.mem, '/' );
  u8* search_rel = search.mem + CsLen( search.mem );

  fsobj_t file = {};
  CsCopy( file.mem, dir_base, dir_base_len );
  CsAddBack( file.mem, '/' );
  u8* file_name_rel_base = file.mem + CsLen( file.mem );

  fsobj_t subdir = {};
  subdir.mem[0] = 0;
  *AddBack( stack_subdirs ) = subdir;

  while( stack_subdirs.len ) {
    subdir = stack_subdirs.mem[stack_subdirs.len - 1];
    RemBack( stack_subdirs );
    idx_t subdir_len = CsLen( subdir.mem );

    u8* file_name_rel = file_name_rel_base; // relative to subdir.
    if( subdir.mem[0] ) {
      CsCopy( file_name_rel, subdir.mem, subdir_len );
      file_name_rel += subdir_len;
      *file_name_rel++ = '/';

      CsCopy( search_rel, subdir.mem, subdir_len );
      CsAddBack( search_rel, '/' );
      CsAddBack( search_rel, '*' );
    } else {
      CsCopy( search_rel, '*' );
    }
    h = FindFirstFile( Cast( char*, search.mem ), &f );
    if( h == INVALID_HANDLE_VALUE )
      continue;

    Forever {
      if( _IsFile( f ) ) {
        CsCopy( file_name_rel, Cast( u8*, f.cFileName ) );
        file.len = CsLen( file.mem );
        *AddBack( dst ) = file;

      } elif( _IsRegularDir( f ) ) {
        u8* subdir_end = subdir.mem + subdir_len;
        if( subdir.mem[0] ) {
          CsCopy( subdir_end, '/' );
          CsAddBack( subdir_end, Cast( u8*, f.cFileName ) );
        } else {
          CsCopy( subdir_end, Cast( u8*, f.cFileName ) );
        }
        *AddBack( stack_subdirs ) = subdir;
        subdir_end[0] = 0; // reset subdir.
      }
      if( !FindNextFile( h, &f ) )
        break;
    }
    FindClose( h );
  }
  Free( stack_subdirs );
}


Inl void
_FindDirsRecur( array_t<fsobj_t>& dst, u8* dir_base, idx_t dir_base_len )
{
  WIN32_FIND_DATA f;
  HANDLE h;

  array_t<fsobj_t> stack_subdirs;
  Alloc( stack_subdirs, 64 );

  fsobj_t search = {};
  CsCopy( search.mem, dir_base, dir_base_len );
  CsAddBack( search.mem, '/' );
  u8* search_rel = search.mem + CsLen( search.mem );

  fsobj_t dir = {};
  CsCopy( dir.mem, dir_base, dir_base_len );
  CsAddBack( dir.mem, '/' );
  u8* dir_name_rel_base = dir.mem + CsLen( dir.mem );

  fsobj_t subdir;
  subdir.len = 0;
  *AddBack( stack_subdirs ) = subdir;

  while( stack_subdirs.len ) {
    subdir = stack_subdirs.mem[stack_subdirs.len - 1];
    RemBack( stack_subdirs );

    u8* dir_name_rel = dir_name_rel_base; // relative to subdir.
    if( subdir.mem[0] ) {
      Memmove( dir_name_rel, subdir.mem, subdir.len );
      dir_name_rel += subdir.len;
      *dir_name_rel++ = '/';

      CsCopy( search_rel, subdir.mem, subdir.len );
      CsAddBack( search_rel, '/' );
      CsAddBack( search_rel, '*' );
    } else {
      CsCopy( search_rel, '*' );
    }
    h = FindFirstFile( Cast( char*, search.mem ), &f );
    if( h == INVALID_HANDLE_VALUE ) {
      continue;
    }

    Forever {
      if( _IsRegularDir( f ) ) {
        CsCopy( dir_name_rel, Cast( u8*, f.cFileName ) );
        dir.len = CsLen( dir.mem );
        *AddBack( dst ) = dir;

        u8* subdir_end = subdir.mem + subdir_len;
        if( subdir.mem[0] ) {
          CsCopy( subdir_end, '/' );
          CsAddBack( subdir_end, Cast( u8*, f.cFileName ) );
        } else {
          CsCopy( subdir_end, Cast( u8*, f.cFileName ) );
        }
        *AddBack( stack_subdirs ) = subdir;
        subdir_end[0] = 0; // reset subdir.
      }
      if( !FindNextFile( h, &f ) ) {
        break;
      }
    }
    FindClose( h );
  }
  Free( stack_subdirs );
}

void
FsFindFiles( array_t<fsobj_t>& dst, u8* dir_base, idx_t dir_base_len, bool recur )
{
  if( recur ) {
    _FindFilesRecur( dst, dir_base, dir_base_len );
  } else {
    _FindFiles( dst, dir_base, dir_base_len );
  }
}

void
FsFindDirs( array_t<fsobj_t>& dst, u8* dir_base, idx_t dir_base_len, bool recur )
{
  if( recur ) {
    _FindDirsRecur( dst, dir_base, dir_base_len );
  } else {
    _FindDirs( dst, dir_base, dir_base_len );
  }
}

#endif









struct
findfiles_t
{
  array_t<slice_t>* spans;
  plist_t* plist;
};
Inl
FS_ITERATOR( _IterFindFiles )
{
  auto dst = Cast( findfiles_t*, misc );
  if( is_file ) {
    auto span = AddBack( *dst->spans );
    span->mem = AddPlist( *dst->plist, u8, 1, len );
    span->len = len;
    Memmove( span->mem, name, len );
  }
  return fsiter_result_t::continue_;
}
Inl void
FsFindFiles(
  array_t<slice_t>& dst,
  plist_t& dstmem,
  u8* path,
  idx_t path_len,
  bool recur
  )
{
  findfiles_t find;
  find.spans = &dst;
  find.plist = &dstmem;
  FsIterate( path, path_len, recur, _IterFindFiles, &find );
}

Inl
FS_ITERATOR( _IterFindDirs )
{
  auto dst = Cast( findfiles_t*, misc );
  if( !is_file ) {
    auto span = AddBack( *dst->spans );
    span->mem = AddPlist( *dst->plist, u8, 1, len );
    span->len = len;
    Memmove( span->mem, name, len );
  }
  return fsiter_result_t::continue_;
}
Inl void
FsFindDirs(
  array_t<slice_t>& dst,
  plist_t& dstmem,
  u8* path,
  idx_t path_len,
  bool recur
  )
{
  findfiles_t find;
  find.spans = &dst;
  find.plist = &dstmem;
  FsIterate( path, path_len, recur, _IterFindDirs, &find );
}




static void
TestFilesys()
{
  {
    fsobj_t cwd;
    FsGetCwd( cwd );
    printf( "cwd: %s\n", cwd.mem );

    fsobj_t newcwd = _StandardDirname( Str( "c:/doc" ), 6 );
    FsSetCwd( newcwd );

    fsobj_t cwd2;
    FsGetCwd( cwd2 );
    AssertCrash( MemEqual( ML( newcwd ), ML( cwd2 ) ) );

    FsSetCwd( cwd );
    FsGetCwd( cwd2 );
    AssertCrash( MemEqual( ML( cwd ), ML( cwd2 ) ) );
  }

  {
    auto cdoc = Str( "c:/doc" );
    auto crand = Str( "c:/docapalooza0123456789" );
    auto crand2 = Str( "c:/docapalooza0123456789/asdf" );
    auto crand3 = Str( "c:/docapalooza0123456789/ghjk" );

    // preconditions:
    AssertCrash( DirExists( cdoc, CsLen( cdoc ) ) );
    AssertCrash( !DirExists( crand, CsLen( crand ) ) );

    // level 0 dir test
    AssertCrash( DirCreate( crand, CsLen( crand ) ) );
    AssertCrash( DirExists( crand, CsLen( crand ) ) );
    AssertCrash( DirDelete( crand, CsLen( crand ) ) );
    AssertCrash( !DirExists( crand, CsLen( crand ) ) );

    // level 1 dir test
    AssertCrash( DirCreate( crand, CsLen( crand ) ) );
    AssertCrash( DirExists( crand, CsLen( crand ) ) );
    AssertCrash( DirCreate( crand2, CsLen( crand2 ) ) );
    AssertCrash( DirExists( crand2, CsLen( crand2 ) ) );
    AssertCrash( DirCreate( crand3, CsLen( crand3 ) ) );
    AssertCrash( DirExists( crand3, CsLen( crand3 ) ) );
    AssertCrash( DirDelete( crand, CsLen( crand ) ) );
    AssertCrash( !DirExists( crand, CsLen( crand ) ) );
    AssertCrash( !DirExists( crand2, CsLen( crand2 ) ) );
    AssertCrash( !DirExists( crand3, CsLen( crand3 ) ) );
  }

#if 0
  u8* name = Str( "c:/doc/dev/cpp/proj/main/exe/64d/test.asm" );
  auto name_len = CsLen( name );
  file_t file = FileOpenExisting( name, name_len, fileop_t::W, fileop_t::R );
  TimeSleep( 2000 );
  printf( "%llu  just opened.\n", file.time_lastwrite );
  FileWrite( file, 0, name, name_len );
  TimeSleep( 2000 );
  printf( "%llu  just executed writes.\n", file.time_lastwrite );
  FileSetEOF( file, name_len );
  TimeSleep( 2000 );
  printf( "%llu  just set file eof.\n", file.time_lastwrite );
  FileFree( file );
  TimeSleep( 2000 );
  printf( "%llu  just closed file handle.\n", FileTimeLastWrite( name, name_len ) );
#endif

  {
    auto path = _fsobj( "c:/dmain/exe" );
    array_t<slice_t> spans;
    Alloc( spans, 65536 );
    plist_t plist;
    Init( plist, 1024*1024 );
    FsFindDirs( spans, plist, ML( path ), 0 );
    Kill( plist );
    Free( spans );
  }

#if 0
  auto path = _fsobj( "c:/" );
  array_t<slice_t> spans;
  Alloc( spans, 65536 );
  plist_t plist;
  Init( plist, 1024*1024 );
  For( i, 0, 4 ) {
    spans.len = 0;
    auto t0 = TimeClock();
    FsFindFiles( spans, plist, ML( path ), 1 );
    auto dt = TimeSecFromClocks32( TimeClock() - t0 );
    printf( "find delta: %f\n", dt );
  }
  Kill( plist );
  Free( spans );
#endif
}
