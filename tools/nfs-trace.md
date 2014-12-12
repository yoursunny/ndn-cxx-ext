# NFS trace replay tool

`nfs-trace-client` and `nfs-trace-server` are a pair of tools to replay NFS trace.  
Client program reads the trace file, expresses Interests where expected server action is injected in Exclude field.  
Server program accepts Interests from clients, and responds according to what's put in Exclude field.

## GETATTR LOOKUP

Interest Name: `ndn:/NFS/{path}/./attr`  
Exclude field: `<NameComponent>ATTR:{ctime}</NameComponent>`

Data Name: `ndn:/NFS/{path}/./attr/{%FD ctime}`  
Content payload: 84 octets

## READLINK

Interest Name: `ndn:/NFS/{path}`  
ChildSelector: rightmost  
Exclude field: `<NameComponent>READLINK:{mtime}</NameComponent>`

Data Name: `ndn:/NFS/{path}/{%FD mtime}`  
Content payload: 160 octets

## READ

Interest Name: `ndn:/NFS/{path}/{%FD mtime}/{%00 segment}`  
MustBeFresh: no  
Exclude field: `<NameComponent>READ:{size}</NameComponent>`

Data Name: same  
Content payload: {size} octets

## WRITE

### client to server

Interest Name: `ndn:/NFS/{path}/./write/{client-host}:{%FD mtime}:{%00 first-seg}:{%00 last-seg}/{signature}`  
Exclude field: `<NameComponent>WRITE</NameComponent>`

Data Name: same  
Content payload: 108 octets

### server to client

Interest Name: `ndn:/{client-host}/NFS/{path}/{%FD mtime}/{%00 seg}`  
MustBeFresh: no  
Exclude field: `<NameComponent>FETCH</NameComponent>`

Data Name: same  
Content payload: 4096 octets

### client to server

Interest Name: `ndn:/NFS/{path}/./commit/{client-host}:{%FD mtime}:{%00 first-seg}:{%00 last-seg}/{signature}`  
Exclude field: `<NameComponent>COMMIT</NameComponent>`

Data Name: same  
Content payload: 100 octets

## READDIRPLUS

### first Interest

Interest Name: `ndn:/NFS/{path}/./dir`  
ChildSelector: rightmost  
Exclude field: `<NameComponent>READDIR1:{mtime}:{nEntries}</NameComponent>`

Data Name: `ndn:/NFS/{path}/./dir/{%FD mtime}/%00%00`  
Content payload: {nEntries * 174} octets

### subsequent Interests

Interest Name: `ndn:/NFS/{path}/./dir/{%FD mtime}/{%00 seg}`  
MustBeFresh: no  
Exclude field: `<NameComponent>READDIR2:{nEntries}</NameComponent>`

Data Name: same  
Content payload: {nEntries * 174} octets

## SETATTR CREATE MKDIR SYMLINK REMOVE RMDIR RENAME

Interest Name: `ndn:/NFS/{path}/./{command}/{args 32 octets}/{signature}`  
Exclude field: `<NameComponent>SIMPLECMD</NameComponent>`

Data Name: same  
Content payload: 248 octets
