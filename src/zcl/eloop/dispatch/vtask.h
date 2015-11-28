/*
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef _Z_VTASK_H_
#define _Z_VTASK_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_vtask_queue z_vtask_queue_t;
typedef struct z_vtask_tree z_vtask_tree_t;
typedef struct z_vtask z_vtask_t;

struct z_vtask {
	struct vtask_link {
		z_vtask_t *child[2];
		z_vtask_t *parent;
	} link;

	uint8_t quantum   : 3;
	uint8_t autoclean : 1;
	uint8_t barrier   : 1;
	uint8_t	cancel    : 1;
	uint8_t rblink    : 1;
	uint8_t attached  : 1;
	uint8_t type;
	uint8_t priority;
	uint8_t uflags8;

	uint32_t vtime;
	uint64_t seqid;
};

struct z_vtask_queue {
  z_vtask_t *head;
};

struct z_vtask_tree {
  z_vtask_t *root;
  z_vtask_t *min_node;
};

void 			 z_vtask_exec 							(z_vtask_t *self);

void       z_vtask_queue_init         (z_vtask_queue_t *self);
void       z_vtask_queue_push         (z_vtask_queue_t *self, z_vtask_t *vtask);
z_vtask_t *z_vtask_queue_pop          (z_vtask_queue_t *self);
void       z_vtask_queue_remove       (z_vtask_queue_t *self, z_vtask_t *vtask);
void       z_vtask_queue_move_to_tail (z_vtask_queue_t *self);
void       z_vtask_queue_cancel       (z_vtask_queue_t *self);
void       z_vtask_queue_dump         (z_vtask_queue_t *self, FILE *stream);
z_vtask_t *z_vtask_queue_next         (z_vtask_t *vtask);

void       z_vtask_tree_init    (z_vtask_tree_t *self);
void       z_vtask_tree_push    (z_vtask_tree_t *self, z_vtask_t *vtask);
z_vtask_t *z_vtask_tree_pop     (z_vtask_tree_t *self);
void       z_vtask_tree_remove  (z_vtask_tree_t *self, z_vtask_t *vtask);
void       z_vtask_tree_cancel  (z_vtask_tree_t *self);
void       z_vtask_tree_dump    (z_vtask_tree_t *self, FILE *stream);
z_vtask_t *z_vtask_tree_next    (z_vtask_t *vtask);
z_vtask_t *z_vtask_tree_prev    (z_vtask_t *vtask);

__Z_END_DECLS__

#endif /* !_Z_VTASK_H_ */