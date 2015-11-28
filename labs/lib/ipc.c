// User-level IPC library routines

#include <inc/lib.h>

// Receive a value via IPC and return it.
// If 'pg' is nonnull, then any page sent by the sender will be mapped at
//	that address.
// If 'from_env_store' is nonnull, then store the IPC sender's envid in
//	*from_env_store.
// If 'perm_store' is nonnull, then store the IPC sender's page permission
//	in *perm_store (this is nonzero iff a page was successfully
//	transferred to 'pg').
// If the system call fails, then store 0 in *fromenv and *perm (if
//	they're nonnull) and return the error.
//
// Hint:
//   Use 'env' to discover the value and who sent it.
//   If 'pg' is null, pass sys_ipc_recv a value that it will understand
//   as meaning "no page".  (Zero is not the right value.)
int32_t
ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
{
	envid_t from_env;
	int from_perm;

	pg = pg ? : (void *)UTOP;
	int r = sys_ipc_recv(pg);
	if (r) {
		from_env = from_perm = 0;
	} else {
		from_env = env->env_ipc_from;
		from_perm = env->env_ipc_perm;
		r = env->env_ipc_value;
	}

	if (from_env_store)
		*from_env_store = from_env;
	if (perm_store)
		*perm_store = from_perm;

	return r;
}

// Send 'val' (and 'pg' with 'perm', if 'pg' is nonnull) to 'toenv'.
// This function keeps trying until it succeeds.
// It should panic() on any error other than -E_IPC_NOT_RECV.
//
// Hint:
//   Use sys_yield() to be CPU-friendly.
//   If 'pg' is null, pass sys_ipc_recv a value that it will understand
//   as meaning "no page".  (Zero is not the right value.)
void
ipc_send(envid_t to_env, uint32_t val, void *pg, int perm)
{
	pg = pg ? : (void *)UTOP;

	for (;;) {
		int r = sys_ipc_try_send(to_env, val, pg, perm);

		switch (r) {
		case 0:
		case 1:
			return;

		case -E_IPC_NOT_RECV:
			sys_yield();
			continue;

		default:
			panic("sys_ipc_try_send(%08x, %u, %08x, %d) failed: %e\n",
				to_env, val, pg, perm, r);
		}
	}
}
