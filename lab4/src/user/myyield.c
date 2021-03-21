#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int id;
	sys_env_set_priority(thisenv->env_id, PRIOR_DEFAULT);
	
	if((id = fork()) != 0)
	{
		sys_env_set_priority(id, PRIOR_SUPREME);

		if((id = fork()) != 0)
		{
			sys_env_set_priority(id, PRIOR_HIGH);

			if((id = fork()) != 0)
			{
				sys_env_set_priority(id, PRIOR_LOW);
			}
		}
	}

	cprintf("Hello, I am environment %08x.\n", thisenv->env_id);
	int i;
	for(i=0; i<3; i++)
	{
		sys_yield();
		cprintf("Back in environment %08x, iteration %d, priority %d\n", sys_getenvid(),i, thisenv->env_priority);
	}
	cprintf("All done in environment %08x.\n", thisenv->env_id);
}