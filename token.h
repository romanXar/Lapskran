#pragma once

enum class CArrayMode {
	TStream,
	Stack
};

template<typename T, CArrayMode Mode = CArrayMode::TStream,	int CAP = 512, int B = 32,int MAX_DEPTH = 64, int MAX_NEW_CHUNKS = 64> 
class CArray {

private:

	static inline uint32_t id_total = 1;
	
	struct Chunk {
		T items[CAP];
		intptr_t live, offset;

		Chunk() {
			live = 0;
			offset = 0;
		}
	};

	struct Node {
		bool  leaf;
		intptr_t   num, count;

		Node** children;
		Chunk** chunks;

		Node(bool is_leaf = false)
		{
			leaf = is_leaf;
			num = count = 0;

			children = nullptr;
			chunks = nullptr;

			if (leaf) chunks = new Chunk * [B + 2];
			else children = new Node * [B + 2];
		}

		~Node()
		{
			if (leaf)
			{
				for (intptr_t i = 0; i < num; ++i) delete chunks[i];
				delete[] chunks;
			}
			else
			{
				for (intptr_t i = 0; i < num; ++i) delete children[i];
				delete[] children;
			}
		}


	};

	struct Tree {
		Node* root, * cache_leaf;
		intptr_t   current_total, old_total, cache_prefix;

		Tree() {

			root = cache_leaf = nullptr;
			current_total = old_total = cache_prefix = 0;

		};

		~Tree() {
			delete root;
		}

	};

	Tree* current = nullptr;


	T& dummy_token() {
		static T t;
		return t;
	}

	void recalc(Node* n) {
		intptr_t s = 0;
		if (n->leaf) {
			for (intptr_t i = 0; i < n->num; i++) s += n->chunks[i]->live;
		}
		else {
			for (intptr_t i = 0; i < n->num; i++) s += n->children[i]->count;
		}
		n->count = s;
	}

	Node* descend(intptr_t i, intptr_t& prefix, intptr_t& local) {
		if (current->cache_leaf) {
			intptr_t first = current->cache_prefix;
			intptr_t last = first + current->cache_leaf->count;
			if (i >= first && i < last) {
				prefix = first;
				local = i - first;
				return current->cache_leaf;
			}
		}

		Node* cur = current->root;
		intptr_t p = 0;
		intptr_t rem = i;
		while (!cur->leaf) {
			intptr_t acc = 0;
			intptr_t k = 0;
			while (k < cur->num - 1) {
				intptr_t c = cur->children[k]->count;
				if (acc + c > rem) break;
				acc += c;
				k++;
			}
			p += acc;
			rem -= acc;
			cur = cur->children[k];
		}

		prefix = p;
		local = rem;
		current->cache_leaf = cur;
		current->cache_prefix = prefix;
		return cur;
	}

	struct DescentPath {
		Node* nodes[MAX_DEPTH];
		intptr_t   depth = 0;
		Node* leaf = nullptr;
		intptr_t   prefix = 0;
		intptr_t   local = 0;

		DescentPath() = default;
	};

	DescentPath descend_path(intptr_t i) {
		DescentPath dp;
		Node* cur = current->root;
		intptr_t p = 0;
		intptr_t rem = i;
		while (!cur->leaf) {
			intptr_t acc = 0;
			intptr_t k = 0;
			while (k < cur->num - 1) {
				intptr_t c = cur->children[k]->count;
				if (acc + c > rem) break;
				acc += c;
				k++;
			}
			dp.nodes[dp.depth++] = cur;
			p += acc;
			rem -= acc;
			cur = cur->children[k];
		}
		dp.leaf = cur;
		dp.prefix = p;
		dp.local = rem;
		return dp;
	}

	void update_path_counts(DescentPath& dp) {
		recalc(dp.leaf);
		for (intptr_t i = dp.depth - 1; i >= 0; i--) recalc(dp.nodes[i]);
	}

	void add_counts_path(DescentPath& dp, intptr_t delta) {
		dp.leaf->count += delta;
		for (intptr_t i = 0; i < dp.depth; i++) {
			dp.nodes[i]->count += delta;
		}
	}


	void replace_chunk_in_leaf(DescentPath& dp, intptr_t k, Chunk** ncs, intptr_t num_chunks) {

		intptr_t acc = 0;
		for (intptr_t i = 0; i < k; i++) acc += dp.leaf->chunks[i]->live;
		intptr_t cur_pos = dp.prefix + acc;

		erase_chunk(cur_pos);

		intptr_t ins_pos = cur_pos;
		for (intptr_t ci = 0; ci < num_chunks; ci++) {
			insert_chunk(ins_pos, ncs[ci]);
			ins_pos += ncs[ci]->live;
		}

		current->cache_leaf = nullptr;
	}

	Node* insert_rec(Node* n, intptr_t elem_pos, Chunk* nc) {
		if (n->leaf) {
			intptr_t acc = 0;
			intptr_t k = 0;
			while (k < n->num) {
				intptr_t c = n->chunks[k]->live;
				if (acc + c > elem_pos) break;
				acc += c;
				k++;
			}
			for (intptr_t i = n->num; i > k; i--) n->chunks[i] = n->chunks[i - 1];
			n->chunks[k] = nc;
			n->num++;
			n->count += nc->live;

			if (n->num > B) {
				Node* right = new Node(true);
				intptr_t mid = n->num / 2;
				for (intptr_t i = mid; i < n->num; i++) right->chunks[i - mid] = n->chunks[i];
				right->num = n->num - mid;
				n->num = mid;
				recalc(n);
				recalc(right);
				return right;
			}
			return nullptr;
		}

		intptr_t acc = 0;
		intptr_t k = 0;
		while (k < n->num - 1) {
			intptr_t c = n->children[k]->count;
			if (acc + c > elem_pos) break;
			acc += c;
			k++;
		}
		intptr_t sub = elem_pos - acc;

		Node* split_node = insert_rec(n->children[k], sub, nc);
		if (split_node) {
			for (intptr_t i = n->num; i > k + 1; i--) n->children[i] = n->children[i - 1];
			n->children[k + 1] = split_node;
			n->num++;
		}
		n->count += nc->live;

		if (n->num > B) {
			Node* right = new Node(false);
			intptr_t mid = n->num / 2;
			for (intptr_t i = mid; i < n->num; i++) right->children[i - mid] = n->children[i];
			right->num = n->num - mid;
			n->num = mid;
			recalc(n);
			recalc(right);
			return right;
		}
		return nullptr;
	}

	void insert_chunk(intptr_t elem_pos, Chunk* nc) {
		if (!current->root) {
			current->root = new Node(true);
			current->root->chunks[0] = nc;
			current->root->num = 1;
			current->root->count = nc->live;
			current->current_total += nc->live;
			current->cache_leaf = nullptr;
			return;
		}
		Node* split_node = insert_rec(current->root, elem_pos, nc);
		if (split_node) {
			Node* new_root = new Node(false);
			new_root->children[0] = current->root;
			new_root->children[1] = split_node;
			new_root->num = 2;
			recalc(new_root);
			current->root = new_root;
		}
		current->current_total += nc->live;
		current->cache_leaf = nullptr;
	}

	void append_chunk(Chunk* nc) {
		if (!current->root) {
			current->root = new Node(true);
			current->root->chunks[0] = nc;
			current->root->num = 1;
			current->root->count = nc->live;
			current->current_total += nc->live;
			current->cache_leaf = nullptr;
			return;
		}

		Node* path[MAX_DEPTH];
		intptr_t depth = 0;
		Node* cur = current->root;
		while (!cur->leaf) {
			path[depth++] = cur;
			cur = cur->children[cur->num - 1];
		}

		cur->chunks[cur->num] = nc;
		cur->num++;
		cur->count += nc->live;

		Node* node = cur;
		while (node->num > B) {
			Node* right = new Node(node->leaf);
			intptr_t mid = node->num / 2;
			if (node->leaf) {
				for (intptr_t i = mid; i < node->num; i++) right->chunks[i - mid] = node->chunks[i];
				right->num = node->num - mid;
				node->num = mid;
			}
			else {
				for (intptr_t i = mid; i < node->num; i++) right->children[i - mid] = node->children[i];
				right->num = node->num - mid;
				node->num = mid;
			}
			recalc(node);
			recalc(right);

			if (depth == 0) {
				Node* new_root = new Node(false);
				new_root->children[0] = node;
				new_root->children[1] = right;
				new_root->num = 2;
				recalc(new_root);
				current->root = new_root;
				node = nullptr;
				break;
			}
			Node* parent = path[--depth];
			parent->children[parent->num] = right;
			parent->num++;
			recalc(parent);
			node = parent;
		}
		for (intptr_t i = depth - 1; i >= 0; i--) recalc(path[i]);
		current->current_total += nc->live;
		current->cache_leaf = nullptr;
	}

	bool erase_rec(Node* n, intptr_t elem_pos) {
		if (n->leaf) {
			intptr_t acc = 0;
			intptr_t k = 0;
			while (k < n->num) {
				intptr_t c = n->chunks[k]->live;
				if (acc + c > elem_pos) break;
				acc += c;
				k++;
			}
			if (k >= n->num) return false;
			intptr_t removed = n->chunks[k]->live;
			delete n->chunks[k];
			for (intptr_t i = k; i < n->num - 1; i++) n->chunks[i] = n->chunks[i + 1];
			n->num--;
			n->count -= removed;
			return n->num == 0;
		}

		intptr_t acc = 0;
		intptr_t k = 0;
		while (k < n->num - 1) {
			intptr_t c = n->children[k]->count;
			if (acc + c > elem_pos) break;
			acc += c;
			k++;
		}
		if (k >= n->num) return false;

		intptr_t old_count = n->children[k]->count;
		bool empty = erase_rec(n->children[k], elem_pos - acc);
		intptr_t new_count = empty ? 0 : n->children[k]->count;
		n->count -= (old_count - new_count);

		if (empty) {
			delete n->children[k];
			for (intptr_t i = k; i < n->num - 1; i++) n->children[i] = n->children[i + 1];
			n->num--;
		}
		return n->num == 0;
	}

	void erase_chunk(intptr_t elem_pos) {
		if (!current->root) return;
		intptr_t prefix, local;
		Node* leaf = descend(elem_pos, prefix, local);
		intptr_t acc = 0;
		intptr_t k = 0;
		while (k < leaf->num) {
			intptr_t c = leaf->chunks[k]->live;
			if (acc + c > local) break;
			acc += c;
			k++;
		}
		intptr_t removed_live = (k < leaf->num) ? leaf->chunks[k]->live : 0;

		bool empty = erase_rec(current->root, elem_pos);
		if (empty) {
			delete current->root;
			current->root = nullptr;
		}
		else if (!current->root->leaf && current->root->num == 1) {
			Node* old = current->root;
			current->root = current->root->children[0];
			old->num = 0;
			delete old;
		}
		current->current_total -= removed_live;
		current->cache_leaf = nullptr;
	}



public:
	
	void EnsureSize(intptr_t target_size) {
		if (target_size <= current->current_total) return; 

		intptr_t n = target_size - current->current_total; 
		current->old_total = current->current_total;

		if (!current->root) {
			intptr_t w = 0;
			while (w < n) {
				Chunk* nc = new Chunk;
				intptr_t take = n - w;
				if (take > CAP) take = CAP;
				nc->live = take; 
				append_chunk(nc);
				w += take;
			}
			current->cache_leaf = nullptr;
			return;
		}

		Node* path[MAX_DEPTH];
		intptr_t depth = 0;
		Node* cur = current->root;
		while (!cur->leaf) {
			path[depth++] = cur;
			cur = cur->children[cur->num - 1];
		}

		intptr_t w = 0;

		if (cur->num > 0) {
			Chunk* last = cur->chunks[cur->num - 1];
			intptr_t avail_end = CAP - last->offset - last->live;
			intptr_t take = (avail_end > n - w) ? n - w : avail_end;

			last->live += take; 
			w += take;

			cur->count += take;
			for (intptr_t p = 0; p < depth; p++) path[p]->count += take;
			current->current_total += take;
		}

		while (w < n) {
			Chunk* nc = new Chunk;
			intptr_t take = n - w;
			if (take > CAP) take = CAP;

			nc->live = take;
			append_chunk(nc);
			w += take;
		}

		current->cache_leaf = nullptr;
	}

	T& get(intptr_t i) {
		if (i < 0 || i >= current->current_total) return dummy_token();
		intptr_t prefix, local;
		Node* leaf = descend(i, prefix, local);
		intptr_t acc = 0;
		for (intptr_t k = 0; k < leaf->num; k++) {
			Chunk* ch = leaf->chunks[k];
			intptr_t c = ch->live;
			if (acc + c > local) return ch->items[ch->offset + (local - acc)];
			acc += c;
		}
		return dummy_token();
	}

	void set(intptr_t i, intptr_t j, const T* src, int& n) {
		current->old_total = current->current_total;

		if (i < 0) i = 0;
		if (j > current->current_total) j = current->current_total;
		if (i > j) i = j;

		if constexpr (Mode == CArrayMode::TStream) {

			if (n > 0 and j < current->current_total and get(j).name == NEWLINE and src[n - 1].name == NEWLINE) {
				current->old_total--;
				j++;
			}
			if (n > 0 and j == current->current_total and src[n - 1].name == NEWLINE) n--;
			if (n > 0 and i > 0 and get(i - 1).name == NEWLINE and src[0].name == NEWLINE) i--;
		}
		if (n > 0 && n == j - i) {
			intptr_t w = 0;
			intptr_t pos = i;
			while (w < n) {
				DescentPath dp = descend_path(pos);
				intptr_t acc = 0;
				intptr_t k = 0;
				while (k < dp.leaf->num) {
					Chunk* ch = dp.leaf->chunks[k];
					if (acc + ch->live > dp.local) break;
					acc += ch->live;
					k++;
				}
				Chunk* ch = dp.leaf->chunks[k];
				intptr_t idx_in_chunk = dp.local - acc;
				intptr_t take = ch->live - idx_in_chunk;
				if (take > n - w) take = n - w;
				for (intptr_t p = 0; p < take; p++)
					ch->items[ch->offset + idx_in_chunk + p] = src[w + p];
				w += take;
				pos += take;
			}
			current->cache_leaf = nullptr;
			return;
		}

		if (j > i) {
			intptr_t left = j - i;
			intptr_t pos = i;
			while (left > 0) {
				DescentPath dp = descend_path(pos);
				intptr_t acc = 0;
				intptr_t k = 0;
				while (k < dp.leaf->num) {
					Chunk* ch = dp.leaf->chunks[k];
					if (acc + ch->live > dp.local) break;
					acc += ch->live;
					k++;
				}
				Chunk* ch = dp.leaf->chunks[k];
				intptr_t idx_in_chunk = dp.local - acc;
				intptr_t avail = ch->live - idx_in_chunk;
				intptr_t take = (avail > left) ? left : avail;

				if (take == ch->live) {
					erase_chunk(pos);
				}
				else {
					intptr_t tail = ch->live - idx_in_chunk - take;
					if (idx_in_chunk < tail) {
						for (intptr_t p = idx_in_chunk - 1; p >= 0; p--)
							ch->items[ch->offset + p + take] = std::move(ch->items[ch->offset + p]);
						ch->offset += take;
					}
					else {
						for (intptr_t p = 0; p < tail; p++)
							ch->items[ch->offset + idx_in_chunk + p] = std::move(ch->items[ch->offset + idx_in_chunk + take + p]);
					}
					ch->live -= take;
					add_counts_path(dp, -take);
					current->current_total -= take;
				}
				left -= take;
			}
			current->cache_leaf = nullptr;
		}

		if (n > 0) {
			if (i == current->current_total || !current->root) {
				intptr_t w = 0;
				if (current->root) {
					Node* path[MAX_DEPTH];
					intptr_t depth = 0;
					Node* cur = current->root;
					while (!cur->leaf) {
						path[depth++] = cur;
						cur = cur->children[cur->num - 1];
					}
					if (cur->num > 0) {
						Chunk* last = cur->chunks[cur->num - 1];
						intptr_t avail_end = CAP - last->offset - last->live;
						intptr_t take = (avail_end > n - w) ? n - w : avail_end;
						for (intptr_t p = 0; p < take; p++)
							last->items[last->offset + last->live + p] = src[w + p];
						last->live += take;
						w += take;
						cur->count += take;
						for (intptr_t p = 0; p < depth; p++) path[p]->count += take;
					}
					current->current_total += w;
				}
				while (w < n) {
					Chunk* nc = new Chunk;
					intptr_t take = n - w;
					if (take > CAP) take = CAP;
					for (intptr_t p = 0; p < take; p++) nc->items[p] = src[w + p];
					nc->live = take;
					append_chunk(nc);
					w += take;
				}
				current->cache_leaf = nullptr;
				return;
			}

			DescentPath dp = descend_path(i);
			intptr_t acc = 0;
			intptr_t k = 0;
			while (k < dp.leaf->num) {
				Chunk* ch = dp.leaf->chunks[k];
				if (acc + ch->live > dp.local) break;
				acc += ch->live;
				k++;
			}
			Chunk* cur = dp.leaf->chunks[k];
			intptr_t cur_offset = dp.local - acc;

			if (cur->live + n <= CAP) {
				intptr_t left_count = cur_offset;
				intptr_t right_count = cur->live - cur_offset;
				intptr_t free_left = cur->offset;
				intptr_t free_right = CAP - cur->offset - cur->live;
				bool can_left = (free_left >= n);
				bool can_right = (free_right >= n);

				bool use_left = can_left && (!can_right || left_count < right_count);
				bool use_right = !use_left && can_right;

				if (use_left) {
					for (intptr_t p = 0; p < left_count; p++)
						cur->items[cur->offset + p - n] = std::move(cur->items[cur->offset + p]);
					cur->offset -= n;
					for (intptr_t p = 0; p < n; p++)
						cur->items[cur->offset + cur_offset + p] = src[p];
					cur->live += n;
					add_counts_path(dp, n);
					current->current_total += n;
					current->cache_leaf = nullptr;
					return;
				}
				if (use_right) {
					for (intptr_t p = right_count - 1; p >= 0; p--)
						cur->items[cur->offset + cur_offset + n + p] = std::move(cur->items[cur->offset + cur_offset + p]);
					for (intptr_t p = 0; p < n; p++)
						cur->items[cur->offset + cur_offset + p] = src[p];
					cur->live += n;
					add_counts_path(dp, n);
					current->current_total += n;
					current->cache_leaf = nullptr;
					return;
				}
			}

			intptr_t old_live = cur->live;
			intptr_t total_new = old_live + n;
			intptr_t num_chunks = (total_new + CAP - 1) / CAP;
			intptr_t cur_src_offset = cur->offset;

			if (num_chunks > MAX_NEW_CHUNKS) {
				std::vector<Chunk*> big_ncs(num_chunks);
				intptr_t per = (total_new + num_chunks - 1) / num_chunks;
				intptr_t pos2 = 0;
				for (intptr_t ci = 0; ci < num_chunks; ci++) {
					intptr_t take = per;
					if (pos2 + take > total_new) take = total_new - pos2;
					Chunk* nc = new Chunk;
					for (intptr_t p = 0; p < take; p++) {
						intptr_t gi = pos2 + p;
						if (gi < cur_offset)          nc->items[p] = std::move(cur->items[cur_src_offset + gi]);
						else if (gi < cur_offset + n) nc->items[p] = src[gi - cur_offset];
						else                          nc->items[p] = std::move(cur->items[cur_src_offset + (gi - n)]);
					}
					nc->live = take;
					big_ncs[ci] = nc;
					pos2 += take;
				}
				intptr_t cur_pos = dp.prefix + acc;
				erase_chunk(cur_pos);
				intptr_t ins_pos = cur_pos;
				for (intptr_t ci = 0; ci < num_chunks; ci++) {
					insert_chunk(ins_pos, big_ncs[ci]);
					ins_pos += big_ncs[ci]->live;
				}
				current->cache_leaf = nullptr;
				return;
			}

			Chunk* ncs[MAX_NEW_CHUNKS];
			intptr_t per = (total_new + num_chunks - 1) / num_chunks;
			intptr_t pos2 = 0;
			for (intptr_t ci = 0; ci < num_chunks; ci++) {
				intptr_t take = per;
				if (pos2 + take > total_new) take = total_new - pos2;
				Chunk* nc = new Chunk;
				for (intptr_t p = 0; p < take; p++) {
					intptr_t gi = pos2 + p;
					if (gi < cur_offset)          nc->items[p] = std::move(cur->items[cur_src_offset + gi]);
					else if (gi < cur_offset + n) nc->items[p] = src[gi - cur_offset];
					else                          nc->items[p] = std::move(cur->items[cur_src_offset + (gi - n)]);
				}
				nc->live = take;
				ncs[ci] = nc;
				pos2 += take;
			}

			replace_chunk_in_leaf(dp, k, ncs, num_chunks);
			current->cache_leaf = nullptr;
		}
	}

	void push_back(const T& t) {
		if (!current->root) {
			Chunk* nc = new Chunk;
			nc->items[0] = t;
			nc->live = 1;
			Node* root = new Node(true);
			root->chunks[0] = nc;
			root->num = 1;
			root->count = 1;
			current->root = root;
			current->current_total = 1;
			return;
		}

		Node* path[MAX_DEPTH];
		intptr_t depth = 0;
		Node* cur = current->root;
		while (!cur->leaf) {
			path[depth++] = cur;
			cur = cur->children[cur->num - 1];
		}

		Chunk* last = cur->chunks[cur->num - 1];

		if constexpr (Mode == CArrayMode::TStream) {

			if (t.name == NEWLINE && last->live > 0 && last->items[last->offset + last->live - 1].name == NEWLINE)
			{
				return;
			}
		}

		if (last->offset + last->live < CAP) {
			last->items[last->offset + last->live] = t;
			last->live++;
			cur->count++;
			for (intptr_t i = depth - 1; i >= 0; i--) path[i]->count++;
			current->current_total++;
			return;
		}

		Chunk* nc = new Chunk;
		nc->items[0] = t;
		nc->live = 1;
		append_chunk(nc);
		return;
	}

	void move_in(CArray* dst_arr, intptr_t i, intptr_t j) {
		Tree* src_tree = current;
		if (!src_tree) return;

		Tree* dst = dst_arr->current;
		if (!dst || dst == src_tree) return;


		intptr_t skip = 0;

		if constexpr (Mode == CArrayMode::TStream) {
			if (i == 0 && src_tree->current_total > 0 && get(0).name == NEWLINE) {
				skip = 1;
			}
		}

		intptr_t n = src_tree->current_total - skip;
		std::vector<T> buf(n);
		for (intptr_t k = 0; k < n; k++) buf[k] = get(k + skip);

		current = dst;
		current->old_total = current->current_total;

		if (i < 0) i = 0;
		if (j > current->current_total) j = current->current_total;
		if (i > j) i = j;


		if constexpr (Mode == CArrayMode::TStream) {

			if (n > 0 and j < current->current_total and get(j).name == NEWLINE and buf[n - 1].name == NEWLINE) {
				current->old_total--;
				j++;
			}

			if (n > 0 and j == current->current_total and buf[n - 1].name == NEWLINE) n--;

			if (n > 0 and i > 0 and get(i - 1).name == NEWLINE and buf[0].name == NEWLINE) {
				i--;
			}
		}
		if (n > 0 && n == j - i) {
			intptr_t w = 0;
			intptr_t pos = i;
			while (w < n) {
				DescentPath dp = descend_path(pos);
				intptr_t acc = 0;
				intptr_t k = 0;
				while (k < dp.leaf->num) {
					Chunk* ch = dp.leaf->chunks[k];
					if (acc + ch->live > dp.local) break;
					acc += ch->live;
					k++;
				}
				Chunk* ch = dp.leaf->chunks[k];
				intptr_t idx_in_chunk = dp.local - acc;
				intptr_t take = ch->live - idx_in_chunk;
				if (take > n - w) take = n - w;
				for (intptr_t p = 0; p < take; p++)
					ch->items[ch->offset + idx_in_chunk + p] = buf[w + p];
				w += take;
				pos += take;
			}
			current->cache_leaf = nullptr;
			goto cleanup;
		}

		if (j > i) {
			intptr_t left = j - i;
			intptr_t pos = i;
			while (left > 0) {
				DescentPath dp = descend_path(pos);
				intptr_t acc = 0;
				intptr_t k = 0;
				while (k < dp.leaf->num) {
					Chunk* ch = dp.leaf->chunks[k];
					if (acc + ch->live > dp.local) break;
					acc += ch->live;
					k++;
				}
				Chunk* ch = dp.leaf->chunks[k];
				intptr_t idx_in_chunk = dp.local - acc;
				intptr_t avail = ch->live - idx_in_chunk;
				intptr_t take = (avail > left) ? left : avail;

				if (take == ch->live) {
					erase_chunk(pos);
				}
				else {
					intptr_t tail = ch->live - idx_in_chunk - take;
					if (idx_in_chunk < tail) {
						for (intptr_t p = idx_in_chunk - 1; p >= 0; p--)
							ch->items[ch->offset + p + take] = std::move(ch->items[ch->offset + p]);
						ch->offset += take;
					}
					else {
						for (intptr_t p = 0; p < tail; p++)
							ch->items[ch->offset + idx_in_chunk + p] = std::move(ch->items[ch->offset + idx_in_chunk + take + p]);
					}
					ch->live -= take;
					add_counts_path(dp, -take);
					current->current_total -= take;
				}
				left -= take;
			}
			current->cache_leaf = nullptr;
		}

		if (n > 0) {
			if (i == current->current_total || !current->root) {
				intptr_t w = 0;
				if (current->root) {
					Node* path[MAX_DEPTH];
					intptr_t depth = 0;
					Node* cur = current->root;
					while (!cur->leaf) {
						path[depth++] = cur;
						cur = cur->children[cur->num - 1];
					}
					if (cur->num > 0) {
						Chunk* last = cur->chunks[cur->num - 1];
						intptr_t avail_end = CAP - last->offset - last->live;
						intptr_t take = (avail_end > n - w) ? n - w : avail_end;
						for (intptr_t p = 0; p < take; p++)
							last->items[last->offset + last->live + p] = buf[w + p];
						last->live += take;
						w += take;
						cur->count += take;
						for (intptr_t p = 0; p < depth; p++) path[p]->count += take;
					}
					current->current_total += w;
				}
				while (w < n) {
					Chunk* nc = new Chunk;
					intptr_t take = n - w;
					if (take > CAP) take = CAP;
					for (intptr_t p = 0; p < take; p++) nc->items[p] = buf[w + p];
					nc->live = take;
					append_chunk(nc);
					w += take;
				}
				current->cache_leaf = nullptr;
				goto cleanup;
			}

			DescentPath dp = descend_path(i);
			intptr_t acc = 0;
			intptr_t k = 0;
			while (k < dp.leaf->num) {
				Chunk* ch = dp.leaf->chunks[k];
				if (acc + ch->live > dp.local) break;
				acc += ch->live;
				k++;
			}
			Chunk* cur = dp.leaf->chunks[k];
			intptr_t cur_offset = dp.local - acc;

			if (cur->live + n <= CAP) {
				intptr_t left_count = cur_offset;
				intptr_t right_count = cur->live - cur_offset;
				intptr_t free_left = cur->offset;
				intptr_t free_right = CAP - cur->offset - cur->live;
				bool can_left = (free_left >= n);
				bool can_right = (free_right >= n);

				bool use_left = can_left && (!can_right || left_count < right_count);
				bool use_right = !use_left && can_right;

				if (use_left) {
					for (intptr_t p = 0; p < left_count; p++)
						cur->items[cur->offset + p - n] = std::move(cur->items[cur->offset + p]);
					cur->offset -= n;
					for (intptr_t p = 0; p < n; p++)
						cur->items[cur->offset + cur_offset + p] = buf[p];
					cur->live += n;
					add_counts_path(dp, n);
					current->current_total += n;
					current->cache_leaf = nullptr;
					goto cleanup;
				}
				if (use_right) {
					for (intptr_t p = right_count - 1; p >= 0; p--)
						cur->items[cur->offset + cur_offset + n + p] = std::move(cur->items[cur->offset + cur_offset + p]);
					for (intptr_t p = 0; p < n; p++)
						cur->items[cur->offset + cur_offset + p] = buf[p];
					cur->live += n;
					add_counts_path(dp, n);
					current->current_total += n;
					current->cache_leaf = nullptr;
					goto cleanup;
				}
			}

			intptr_t old_live = cur->live;
			intptr_t total_new = old_live + n;
			intptr_t num_chunks = (total_new + CAP - 1) / CAP;
			intptr_t cur_src_offset = cur->offset;

			if (num_chunks > MAX_NEW_CHUNKS) {
				std::vector<Chunk*> big_ncs(num_chunks);
				intptr_t per = (total_new + num_chunks - 1) / num_chunks;
				intptr_t pos2 = 0;
				for (intptr_t ci = 0; ci < num_chunks; ci++) {
					intptr_t take = per;
					if (pos2 + take > total_new) take = total_new - pos2;
					Chunk* nc = new Chunk;
					for (intptr_t p = 0; p < take; p++) {
						intptr_t gi = pos2 + p;
						if (gi < cur_offset)          nc->items[p] = std::move(cur->items[cur_src_offset + gi]);
						else if (gi < cur_offset + n) nc->items[p] = buf[gi - cur_offset];
						else                          nc->items[p] = std::move(cur->items[cur_src_offset + (gi - n)]);
					}
					nc->live = take;
					big_ncs[ci] = nc;
					pos2 += take;
				}
				intptr_t cur_pos = dp.prefix + acc;
				erase_chunk(cur_pos);
				intptr_t ins_pos = cur_pos;
				for (intptr_t ci = 0; ci < num_chunks; ci++) {
					insert_chunk(ins_pos, big_ncs[ci]);
					ins_pos += big_ncs[ci]->live;
				}
				current->cache_leaf = nullptr;
				goto cleanup;
			}

			Chunk* ncs[MAX_NEW_CHUNKS];
			intptr_t per = (total_new + num_chunks - 1) / num_chunks;
			intptr_t pos2 = 0;
			for (intptr_t ci = 0; ci < num_chunks; ci++) {
				intptr_t take = per;
				if (pos2 + take > total_new) take = total_new - pos2;
				Chunk* nc = new Chunk;
				for (intptr_t p = 0; p < take; p++) {
					intptr_t gi = pos2 + p;
					if (gi < cur_offset)          nc->items[p] = std::move(cur->items[cur_src_offset + gi]);
					else if (gi < cur_offset + n) nc->items[p] = buf[gi - cur_offset];
					else                          nc->items[p] = std::move(cur->items[cur_src_offset + (gi - n)]);
				}
				nc->live = take;
				ncs[ci] = nc;
				pos2 += take;
			}

			replace_chunk_in_leaf(dp, k, ncs, num_chunks);
			current->cache_leaf = nullptr;
		}

	cleanup:
		delete src_tree;
		current = new Tree();
	}

	uintptr_t get_length() {
		return current->current_total;
	}

	uintptr_t get_resize() {
		return current->current_total - current->old_total;
	}
	static uint32_t gen_id() {
		return id_total++;
	}

	CArray() {
		current = new Tree();
	}

	~CArray() {
		delete current;
	}

};

