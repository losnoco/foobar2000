/*
 * bs1770_stats.c
 * Copyright (C) 2011, 2012 Peter Belkner <pbelkner@snafu.de>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */
#include <string.h>
#include "bs1770.h"

bs1770_stats_t *bs1770_stats_init(bs1770_stats_t *stats, bs1770_hist_t *album,
    const bs1770_ps_t *ps)
{
  memset(stats,0,sizeof *stats);
  stats->album=album;

  if (NULL==bs1770_hist_init(&stats->track,ps))
	goto error;
  else if (NULL==bs1770_aggr_init(&stats->aggr,ps,&stats->track,album))
	goto error;

  stats->active=1;

  return stats;
error:
  bs1770_stats_cleanup(stats);

  return NULL;
}

bs1770_stats_t *bs1770_stats_cleanup(bs1770_stats_t *stats)
{
  bs1770_aggr_cleanup(&stats->aggr);
  bs1770_hist_cleanup(&stats->track);

  return stats;
}
