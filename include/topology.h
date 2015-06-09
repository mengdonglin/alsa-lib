/*
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  Copyright (C) 2015 Intel Corporation
 *
 */

#ifndef __ALSA_TOPOLOGY_H
#define __ALSA_TOPOLOGY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \defgroup Topology Interface
 *  The topology interface.
 *  See \ref Topology page for more details.
 *  \{
 */

/*! \page Topology ALSA Topology Interface
 * 
 * ALSA Use Case Interface
 *
 */

typedef struct snd_tplg snd_tplg_t;

snd_tplg_t *snd_socfw_new(const char *name, int verbose);
void snd_socfw_free(snd_tplg_t *tplg);

int snd_parse_conf(snd_tplg_t *tplg, const char *filename);


#ifdef __cplusplus
}
#endif

#endif /* __ALSA_TOPOLOGY_H */
