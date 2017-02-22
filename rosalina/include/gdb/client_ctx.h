
enum gdb_flags
{	
	GDB_FLAG_EMPTY = 0,
	GDB_FLAG_USED  = 1,
};

struct gdb_client_ctx
{
	enum gdb_flags flags;
};

void gdb_setup_client(struct gdb_client_ctx *ctx);
void gdb_destroy_client(struct gdb_client_ctx *ctx);
int gdb_do_packet(Handle socket, struct gdb_client_ctx *ctx);