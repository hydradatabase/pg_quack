#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "postgres.h"

#include "miscadmin.h"

#include "access/amapi.h"
#include "commands/vacuum.h"
#include "utils/guc.h"

#include "quack.h"

PG_MODULE_MAGIC;

char * quack_data_dir = NULL;

static void quack_check_data_directory(void);

void
_PG_init(void)
{
  DefineCustomStringVariable("quack.data_dir",
                             gettext_noop("Quack storage data directory."),
                             NULL,
                             &quack_data_dir,
                             "/opt/quack/",
                             PGC_BACKEND, GUC_IS_NAME,
                             NULL,
                             NULL,
                             NULL);


  quack_check_data_directory();

  quack_init_tableam();
  quack_init_hooks();
}

void
quack_check_data_directory(void)
{
  struct stat info;

  if(lstat(quack_data_dir, &info) != 0)
  {
    if(errno == ENOENT)
    {
      elog(ERROR, "Directory `%s` doesn't exists.", quack_data_dir);
    } 
    else if(errno == EACCES)
    {
      elog(ERROR, "Can't access `%s` directory.", quack_data_dir);
    }
    else
    {
      elog(ERROR, "Other error when reading `%s`.", quack_data_dir);
    }
  }

  if(!S_ISDIR(info.st_mode))
  {
    elog(WARNING, "`%s` is not directory.", quack_data_dir);
  }

  if (access(quack_data_dir, R_OK | W_OK))
  {
    elog(ERROR, "Directory `%s` permission problem.", quack_data_dir);
  }
}