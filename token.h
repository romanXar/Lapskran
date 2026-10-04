#pragma once

namespace btree {

	uint32_t id_total = 1;
	const int CAP = 512;
	const int B = 32;
	const int MAX_DEPTH = 64;
	const int MAX_NEW_CHUNKS = 64;
	
	struct Token {
		uint32_t id;
		std::string name;
	};
	
	struct Chunk {
		Token items[CAP];
		int live, offset;

		Chunk() {
			live = 0;
			offset = 0;
		}
	};

	struct Node {
		bool  leaf;
		int   num,count;

		Node** children;
		Chunk** chunks;

		Node(bool is_leaf = false) 
		{
			leaf = is_leaf;
			num = count = 0;
			
			children = nullptr;
			chunks = nullptr;

			if (leaf) chunks = new Chunk *[B + 2];
			else children = new Node *[B + 2];
		}

		~Node() 
		{
			if (leaf) 
			{
				for (int i = 0; i < num; ++i) delete chunks[i];
				delete[] chunks;
			}
			else 
			{
				for (int i = 0; i < num; ++i) delete children[i];
				delete[] children;
			}
		}


	};

	struct Tree {
		Node *root, *cache_leaf;
		int   current_total,old_total,cache_prefix;
		
		Tree() {

			root = cache_leaf = nullptr;
			current_total = old_total = cache_prefix = 0;
			
		};

		~Tree() {
			delete root;
		}

	};

	inline std::unordered_map<std::string, Tree*> trees;
	inline Tree* current = nullptr;

	inline int get_resize() {
		return current->current_total - current->old_total;
	}

	inline void create_tree(std::string name) {
		trees[name] = new Tree;
	}

	inline void use(std::string name) {
		current = trees[name];
	}

	inline unsigned int get_length() {
		return current->current_total;
	}

	inline void destroy_tree(std::string name) {
		delete trees[name];
		trees[name] = nullptr;
	}

	inline Token& dummy_token() {
		static Token t;
		return t;
	}

	inline void recalc(Node* n) {
		int s = 0;
		if (n->leaf) {
			for (int i = 0; i < n->num; i++) s += n->chunks[i]->live;
		}
		else {
			for (int i = 0; i < n->num; i++) s += n->children[i]->count;
		}
		n->count = s;
	}

	inline Node* descend(int i, int& prefix, int& local) {
		if (current->cache_leaf) {
			int first = current->cache_prefix;
			int last = first + current->cache_leaf->count;
			if (i >= first && i < last) {
				prefix = first;
				local = i - first;
				return current->cache_leaf;
			}
		}

		Node* cur = current->root;
		int p = 0;
		int rem = i;
		while (!cur->leaf) {
			int acc = 0;
			int k = 0;
			while (k < cur->num - 1) {
				int c = cur->children[k]->count;
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
		int   depth = 0;
		Node* leaf = nullptr;
		int   prefix = 0;
		int   local = 0;

		DescentPath() = default;
	};

	inline DescentPath descend_path(int i) {
		DescentPath dp;
		Node* cur = current->root;
		int p = 0;
		int rem = i;
		while (!cur->leaf) {
			int acc = 0;
			int k = 0;
			while (k < cur->num - 1) {
				int c = cur->children[k]->count;
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

	inline void update_path_counts(DescentPath& dp) {
		recalc(dp.leaf);
		for (int i = dp.depth - 1; i >= 0; i--) recalc(dp.nodes[i]);
	}

	inline void add_counts_path(DescentPath& dp, int delta) {
		dp.leaf->count += delta;
		for (int i = 0; i < dp.depth; i++) {
			dp.nodes[i]->count += delta;
		}
	}

	inline void replace_chunk_in_leaf(DescentPath& dp, int k,
		Chunk** ncs, int num_chunks) {
		Node* leaf = dp.leaf;

		int delta = 0;
		for (int i = 0; i < num_chunks; i++) delta += ncs[i]->live;
		delta -= leaf->chunks[k]->live;

		delete leaf->chunks[k];
		for (int i = k; i < leaf->num - 1; i++)
			leaf->chunks[i] = leaf->chunks[i + 1];
		leaf->num--;

		for (int i = leaf->num - 1; i >= k; i--)
			leaf->chunks[i + num_chunks] = leaf->chunks[i];

		for (int ci = 0; ci < num_chunks; ci++)
			leaf->chunks[k + ci] = ncs[ci];
		leaf->num += num_chunks;

		leaf->count += delta;
		for (int i = 0; i < dp.depth; i++)
			dp.nodes[i]->count += delta;

		Node* node = leaf;
		int d = dp.depth - 1;
		while (node->num > B) {
			Node* right = new Node(node->leaf);
			int mid = node->num / 2;
			if (node->leaf) {
				for (int i = mid; i < node->num; i++)
					right->chunks[i - mid] = node->chunks[i];
				right->num = node->num - mid;
				node->num = mid;
			}
			else {
				for (int i = mid; i < node->num; i++)
					right->children[i - mid] = node->children[i];
				right->num = node->num - mid;
				node->num = mid;
			}
			recalc(node);
			recalc(right);

			if (d < 0) {
				Node* new_root = new Node(false);
				new_root->children[0] = node;
				new_root->children[1] = right;
				new_root->num = 2;
				recalc(new_root);
				current->root = new_root;
				return;
			}

			Node* parent = dp.nodes[d];
			int idx = 0;
			while (parent->children[idx] != node) idx++;

			for (int i = parent->num; i > idx + 1; i--)
				parent->children[i] = parent->children[i - 1];
			parent->children[idx + 1] = right;
			parent->num++;

			node = parent;
			d--;
		}
	}

	Node* insert_rec(Node* n, int elem_pos, Chunk* nc) {
		if (n->leaf) {
			int acc = 0;
			int k = 0;
			while (k < n->num) {
				int c = n->chunks[k]->live;
				if (acc + c > elem_pos) break;
				acc += c;
				k++;
			}
			for (int i = n->num; i > k; i--) n->chunks[i] = n->chunks[i - 1];
			n->chunks[k] = nc;
			n->num++;
			n->count += nc->live;

			if (n->num > B) {
				Node* right = new Node(true);
				int mid = n->num / 2;
				for (int i = mid; i < n->num; i++) right->chunks[i - mid] = n->chunks[i];
				right->num = n->num - mid;
				n->num = mid;
				recalc(n);
				recalc(right);
				return right;
			}
			return nullptr;
		}

		int acc = 0;
		int k = 0;
		while (k < n->num - 1) {
			int c = n->children[k]->count;
			if (acc + c > elem_pos) break;
			acc += c;
			k++;
		}
		int sub = elem_pos - acc;

		Node* split_node = insert_rec(n->children[k], sub, nc);
		if (split_node) {
			for (int i = n->num; i > k + 1; i--) n->children[i] = n->children[i - 1];
			n->children[k + 1] = split_node;
			n->num++;
		}
		n->count += nc->live;

		if (n->num > B) {
			Node* right = new Node(false);
			int mid = n->num / 2;
			for (int i = mid; i < n->num; i++) right->children[i - mid] = n->children[i];
			right->num = n->num - mid;
			n->num = mid;
			recalc(n);
			recalc(right);
			return right;
		}
		return nullptr;
	}

	inline void insert_chunk(int elem_pos, Chunk* nc) {
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
		int depth = 0;
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
			int mid = node->num / 2;
			if (node->leaf) {
				for (int i = mid; i < node->num; i++) right->chunks[i - mid] = node->chunks[i];
				right->num = node->num - mid;
				node->num = mid;
			}
			else {
				for (int i = mid; i < node->num; i++) right->children[i - mid] = node->children[i];
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
		for (int i = depth - 1; i >= 0; i--) recalc(path[i]);
		current->current_total += nc->live;
		current->cache_leaf = nullptr;
	}

	bool erase_rec(Node* n, int elem_pos) {
		if (n->leaf) {
			int acc = 0;
			int k = 0;
			while (k < n->num) {
				int c = n->chunks[k]->live;
				if (acc + c > elem_pos) break;
				acc += c;
				k++;
			}
			if (k >= n->num) return false;
			int removed = n->chunks[k]->live;
			delete n->chunks[k];
			for (int i = k; i < n->num - 1; i++) n->chunks[i] = n->chunks[i + 1];
			n->num--;
			n->count -= removed;
			return n->num == 0;
		}

		int acc = 0;
		int k = 0;
		while (k < n->num - 1) {
			int c = n->children[k]->count;
			if (acc + c > elem_pos) break;
			acc += c;
			k++;
		}
		if (k >= n->num) return false;

		int old_count = n->children[k]->count;
		bool empty = erase_rec(n->children[k], elem_pos - acc);
		int new_count = empty ? 0 : n->children[k]->count;
		n->count -= (old_count - new_count);

		if (empty) {
			delete n->children[k];
			for (int i = k; i < n->num - 1; i++) n->children[i] = n->children[i + 1];
			n->num--;
		}
		return n->num == 0;
	}

	inline void erase_chunk(int elem_pos) {
		if (!current->root) return;
		int prefix, local;
		Node* leaf = descend(elem_pos, prefix, local);
		int acc = 0;
		int k = 0;
		while (k < leaf->num) {
			int c = leaf->chunks[k]->live;
			if (acc + c > local) break;
			acc += c;
			k++;
		}
		int removed_live = (k < leaf->num) ? leaf->chunks[k]->live : 0;

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

	inline Token& get(int i) {
		if (i < 0 || i >= current->current_total) return dummy_token();
		int prefix, local;
		Node* leaf = descend(i, prefix, local);
		int acc = 0;
		for (int k = 0; k < leaf->num; k++) {
			Chunk* ch = leaf->chunks[k];
			int c = ch->live;
			if (acc + c > local) return ch->items[ch->offset + (local - acc)];
			acc += c;
		}
		return dummy_token();
	}

	inline void set(int i, int j, const Token* src, int& n) {
		current->old_total = current->current_total;

		if (i < 0) i = 0;
		if (j > current->current_total) j = current->current_total;
		if (i > j) i = j;
		
		if (n > 0 and j < current->current_total and get(j).name == "\n" and src[n - 1].name == "\n") {
			current->old_total--;
			j++;
		}
		if (n > 0 and j == current->current_total and src[n - 1].name == "\n") n--;
		if (n > 0 and i > 0 and get(i - 1).name == "\n" and src[0].name == "\n") i--;
		
		// ---------- in-place ----------
		if (n > 0 && n == j - i) {
			int w = 0;
			int pos = i;
			while (w < n) {
				DescentPath dp = descend_path(pos);
				int acc = 0;
				int k = 0;
				while (k < dp.leaf->num) {
					Chunk* ch = dp.leaf->chunks[k];
					if (acc + ch->live > dp.local) break;
					acc += ch->live;
					k++;
				}
				Chunk* ch = dp.leaf->chunks[k];
				int idx_in_chunk = dp.local - acc;
				int take = ch->live - idx_in_chunk;
				if (take > n - w) take = n - w;
				for (int p = 0; p < take; p++)
					ch->items[ch->offset + idx_in_chunk + p] = src[w + p];
				w += take;
				pos += take;
			}
			current->cache_leaf = nullptr;
			return;
		}

		// ---------- erase [i, j) ----------
		if (j > i) {
			int left = j - i;
			int pos = i;
			while (left > 0) {
				DescentPath dp = descend_path(pos);
				int acc = 0;
				int k = 0;
				while (k < dp.leaf->num) {
					Chunk* ch = dp.leaf->chunks[k];
					if (acc + ch->live > dp.local) break;
					acc += ch->live;
					k++;
				}
				Chunk* ch = dp.leaf->chunks[k];
				int idx_in_chunk = dp.local - acc;
				int avail = ch->live - idx_in_chunk;
				int take = (avail > left) ? left : avail;

				if (take == ch->live) {
					erase_chunk(pos);
				}
				else {
					int tail = ch->live - idx_in_chunk - take;
					if (idx_in_chunk < tail) {
						for (int p = idx_in_chunk - 1; p >= 0; p--)
							ch->items[ch->offset + p + take] = std::move(ch->items[ch->offset + p]);
						ch->offset += take;
					}
					else {
						for (int p = 0; p < tail; p++)
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

		// ---------- insert n в позицию i ----------
		if (n > 0) {
			if (i == current->current_total || !current->root) {
				int w = 0;
				if (current->root) {
					Node* path[MAX_DEPTH];
					int depth = 0;
					Node* cur = current->root;
					while (!cur->leaf) {
						path[depth++] = cur;
						cur = cur->children[cur->num - 1];
					}
					if (cur->num > 0) {
						Chunk* last = cur->chunks[cur->num - 1];
						int avail_end = CAP - last->offset - last->live;
						int take = (avail_end > n - w) ? n - w : avail_end;
						for (int p = 0; p < take; p++)
							last->items[last->offset + last->live + p] = src[w + p];
						last->live += take;
						w += take;
						cur->count += take;
						for (int p = 0; p < depth; p++) path[p]->count += take;
					}
					current->current_total += w;
				}
				while (w < n) {
					Chunk* nc = new Chunk;
					int take = n - w;
					if (take > CAP) take = CAP;
					for (int p = 0; p < take; p++) nc->items[p] = src[w + p];
					nc->live = take;
					append_chunk(nc);
					w += take;
				}
				current->cache_leaf = nullptr;
				return;
			}

			DescentPath dp = descend_path(i);
			int acc = 0;
			int k = 0;
			while (k < dp.leaf->num) {
				Chunk* ch = dp.leaf->chunks[k];
				if (acc + ch->live > dp.local) break;
				acc += ch->live;
				k++;
			}
			Chunk* cur = dp.leaf->chunks[k];
			int cur_offset = dp.local - acc;

			if (cur->live + n <= CAP) {
				int left_count = cur_offset;
				int right_count = cur->live - cur_offset;
				int free_left = cur->offset;
				int free_right = CAP - cur->offset - cur->live;
				bool can_left = (free_left >= n);
				bool can_right = (free_right >= n);

				bool use_left = can_left && (!can_right || left_count < right_count);
				bool use_right = !use_left && can_right;

				if (use_left) {
					for (int p = 0; p < left_count; p++)
						cur->items[cur->offset + p - n] = std::move(cur->items[cur->offset + p]);
					cur->offset -= n;
					for (int p = 0; p < n; p++)
						cur->items[cur->offset + cur_offset + p] = src[p];
					cur->live += n;
					add_counts_path(dp, n);
					current->current_total += n;
					current->cache_leaf = nullptr;
					return;
				}
				if (use_right) {
					for (int p = right_count - 1; p >= 0; p--)
						cur->items[cur->offset + cur_offset + n + p] = std::move(cur->items[cur->offset + cur_offset + p]);
					for (int p = 0; p < n; p++)
						cur->items[cur->offset + cur_offset + p] = src[p];
					cur->live += n;
					add_counts_path(dp, n);
					current->current_total += n;
					current->cache_leaf = nullptr;
					return;
				}
			}

			int old_live = cur->live;
			int total_new = old_live + n;
			int num_chunks = (total_new + CAP - 1) / CAP;
			int cur_src_offset = cur->offset;

			if (num_chunks > MAX_NEW_CHUNKS) {
				std::vector<Chunk*> big_ncs(num_chunks);
				int per = (total_new + num_chunks - 1) / num_chunks;
				int pos2 = 0;
				for (int ci = 0; ci < num_chunks; ci++) {
					int take = per;
					if (pos2 + take > total_new) take = total_new - pos2;
					Chunk* nc = new Chunk;
					for (int p = 0; p < take; p++) {
						int gi = pos2 + p;
						if (gi < cur_offset)          nc->items[p] = std::move(cur->items[cur_src_offset + gi]);
						else if (gi < cur_offset + n) nc->items[p] = src[gi - cur_offset];
						else                          nc->items[p] = std::move(cur->items[cur_src_offset + (gi - n)]);
					}
					nc->live = take;
					big_ncs[ci] = nc;
					pos2 += take;
				}
				int cur_pos = dp.prefix + acc;
				erase_chunk(cur_pos);
				int ins_pos = cur_pos;
				for (int ci = 0; ci < num_chunks; ci++) {
					insert_chunk(ins_pos, big_ncs[ci]);
					ins_pos += big_ncs[ci]->live;
				}
				current->cache_leaf = nullptr;
				return;
			}

			Chunk* ncs[MAX_NEW_CHUNKS];
			int per = (total_new + num_chunks - 1) / num_chunks;
			int pos2 = 0;
			for (int ci = 0; ci < num_chunks; ci++) {
				int take = per;
				if (pos2 + take > total_new) take = total_new - pos2;
				Chunk* nc = new Chunk;
				for (int p = 0; p < take; p++) {
					int gi = pos2 + p;
					if (gi < cur_offset)          nc->items[p] = std::move(cur->items[cur_src_offset + gi]);
					else if (gi < cur_offset + n) nc->items[p] = src[gi - cur_offset];
					else                          nc->items[p] = std::move(cur->items[cur_src_offset + (gi - n)]);
				}
				nc->live = take;
				ncs[ci] = nc;
				pos2 += take;
			}

			current->current_total += n;
			replace_chunk_in_leaf(dp, k, ncs, num_chunks);
			current->cache_leaf = nullptr;
		}
	}

	inline void push_back(const Token& t) {
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
		int depth = 0;
		Node* cur = current->root;
		while (!cur->leaf) {
			path[depth++] = cur;
			cur = cur->children[cur->num - 1];
		}

		Chunk* last = cur->chunks[cur->num - 1];

		// Симметрично move_in: если вставляем '\n' в самый конец
		// и последний символ уже '\n' — это no-op.
		if (t.name == "\n"
			&& last->live > 0
			&& last->items[last->offset + last->live - 1].name == "\n")
		{
			return;
		}
		
		if (last->offset + last->live < CAP) {
			last->items[last->offset + last->live] = t;
			last->live++;
			cur->count++;
			for (int i = depth - 1; i >= 0; i--) path[i]->count++;
			current->current_total++;
			return;
		}

		Chunk* nc = new Chunk;
		nc->items[0] = t;
		nc->live = 1;
		append_chunk(nc);
		return;
	}

	inline void move_in(std::string dst_name, int i, int j) {
		Tree* src_tree = current;
		if (!src_tree) return;

		Tree* dst = trees[dst_name];
		if (!dst || dst == src_tree) return;

		std::string src_name;
		for (auto& [n, t] : trees) {
			if (t == src_tree) { src_name = n; break; }
		}
		
		int skip = (i == 0 && src_tree->current_total > 0 && get(0).name == "\n") ? 1 : 0;
		int n = src_tree->current_total - skip;
		std::vector<Token> buf(n);
		for (int k = 0; k < n; k++) buf[k] = get(k + skip);
		
		//////////////////////////
		//int n = src_tree->current_total;

		//std::vector<Token> buf(n);
		//for (int k = 0; k < n; k++) buf[k] = get(k);
		/////////////////////////////////////
		current = dst;
		current->old_total = current->current_total;

		if (i < 0) i = 0;
		if (j > current->current_total) j = current->current_total;
		if (i > j) i = j;
		
		if (n > 0 and j < current->current_total and get(j).name == "\n" and buf[n - 1].name == "\n") {
			current->old_total--;
			j++;
		}

		if (n > 0 and j == current->current_total and buf[n - 1].name == "\n") n--;

		if (n > 0 and i > 0 and get(i - 1).name == "\n" and buf[0].name == "\n") {
			i--;
		}
		
		if (n > 0 && n == j - i) {
			int w = 0;
			int pos = i;
			while (w < n) {
				DescentPath dp = descend_path(pos);
				int acc = 0;
				int k = 0;
				while (k < dp.leaf->num) {
					Chunk* ch = dp.leaf->chunks[k];
					if (acc + ch->live > dp.local) break;
					acc += ch->live;
					k++;
				}
				Chunk* ch = dp.leaf->chunks[k];
				int idx_in_chunk = dp.local - acc;
				int take = ch->live - idx_in_chunk;
				if (take > n - w) take = n - w;
				for (int p = 0; p < take; p++)
					ch->items[ch->offset + idx_in_chunk + p] = buf[w + p];
				w += take;
				pos += take;
			}
			current->cache_leaf = nullptr;
			goto cleanup;
		}

		if (j > i) {
			int left = j - i;
			int pos = i;
			while (left > 0) {
				DescentPath dp = descend_path(pos);
				int acc = 0;
				int k = 0;
				while (k < dp.leaf->num) {
					Chunk* ch = dp.leaf->chunks[k];
					if (acc + ch->live > dp.local) break;
					acc += ch->live;
					k++;
				}
				Chunk* ch = dp.leaf->chunks[k];
				int idx_in_chunk = dp.local - acc;
				int avail = ch->live - idx_in_chunk;
				int take = (avail > left) ? left : avail;

				if (take == ch->live) {
					erase_chunk(pos);
				}
				else {
					int tail = ch->live - idx_in_chunk - take;
					if (idx_in_chunk < tail) {
						for (int p = idx_in_chunk - 1; p >= 0; p--)
							ch->items[ch->offset + p + take] = std::move(ch->items[ch->offset + p]);
						ch->offset += take;
					}
					else {
						for (int p = 0; p < tail; p++)
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
				int w = 0;
				if (current->root) {
					Node* path[MAX_DEPTH];
					int depth = 0;
					Node* cur = current->root;
					while (!cur->leaf) {
						path[depth++] = cur;
						cur = cur->children[cur->num - 1];
					}
					if (cur->num > 0) {
						Chunk* last = cur->chunks[cur->num - 1];
						int avail_end = CAP - last->offset - last->live;
						int take = (avail_end > n - w) ? n - w : avail_end;
						for (int p = 0; p < take; p++)
							last->items[last->offset + last->live + p] = buf[w + p];
						last->live += take;
						w += take;
						cur->count += take;
						for (int p = 0; p < depth; p++) path[p]->count += take;
					}
					current->current_total += w;
				}
				while (w < n) {
					Chunk* nc = new Chunk;
					int take = n - w;
					if (take > CAP) take = CAP;
					for (int p = 0; p < take; p++) nc->items[p] = buf[w + p];
					nc->live = take;
					append_chunk(nc);
					w += take;
				}
				current->cache_leaf = nullptr;
				goto cleanup;
			}

			DescentPath dp = descend_path(i);
			int acc = 0;
			int k = 0;
			while (k < dp.leaf->num) {
				Chunk* ch = dp.leaf->chunks[k];
				if (acc + ch->live > dp.local) break;
				acc += ch->live;
				k++;
			}
			Chunk* cur = dp.leaf->chunks[k];
			int cur_offset = dp.local - acc;

			if (cur->live + n <= CAP) {
				int left_count = cur_offset;
				int right_count = cur->live - cur_offset;
				int free_left = cur->offset;
				int free_right = CAP - cur->offset - cur->live;
				bool can_left = (free_left >= n);
				bool can_right = (free_right >= n);

				bool use_left = can_left && (!can_right || left_count < right_count);
				bool use_right = !use_left && can_right;

				if (use_left) {
					for (int p = 0; p < left_count; p++)
						cur->items[cur->offset + p - n] = std::move(cur->items[cur->offset + p]);
					cur->offset -= n;
					for (int p = 0; p < n; p++)
						cur->items[cur->offset + cur_offset + p] = buf[p];
					cur->live += n;
					add_counts_path(dp, n);
					current->current_total += n;
					current->cache_leaf = nullptr;
					goto cleanup;
				}
				if (use_right) {
					for (int p = right_count - 1; p >= 0; p--)
						cur->items[cur->offset + cur_offset + n + p] = std::move(cur->items[cur->offset + cur_offset + p]);
					for (int p = 0; p < n; p++)
						cur->items[cur->offset + cur_offset + p] = buf[p];
					cur->live += n;
					add_counts_path(dp, n);
					current->current_total += n;
					current->cache_leaf = nullptr;
					goto cleanup;
				}
			}

			int old_live = cur->live;
			int total_new = old_live + n;
			int num_chunks = (total_new + CAP - 1) / CAP;
			int cur_src_offset = cur->offset;

			if (num_chunks > MAX_NEW_CHUNKS) {
				std::vector<Chunk*> big_ncs(num_chunks);
				int per = (total_new + num_chunks - 1) / num_chunks;
				int pos2 = 0;
				for (int ci = 0; ci < num_chunks; ci++) {
					int take = per;
					if (pos2 + take > total_new) take = total_new - pos2;
					Chunk* nc = new Chunk;
					for (int p = 0; p < take; p++) {
						int gi = pos2 + p;
						if (gi < cur_offset)          nc->items[p] = std::move(cur->items[cur_src_offset + gi]);
						else if (gi < cur_offset + n) nc->items[p] = buf[gi - cur_offset];
						else                          nc->items[p] = std::move(cur->items[cur_src_offset + (gi - n)]);
					}
					nc->live = take;
					big_ncs[ci] = nc;
					pos2 += take;
				}
				int cur_pos = dp.prefix + acc;
				erase_chunk(cur_pos);
				int ins_pos = cur_pos;
				for (int ci = 0; ci < num_chunks; ci++) {
					insert_chunk(ins_pos, big_ncs[ci]);
					ins_pos += big_ncs[ci]->live;
				}
				current->cache_leaf = nullptr;
				goto cleanup;
			}

			Chunk* ncs[MAX_NEW_CHUNKS];
			int per = (total_new + num_chunks - 1) / num_chunks;
			int pos2 = 0;
			for (int ci = 0; ci < num_chunks; ci++) {
				int take = per;
				if (pos2 + take > total_new) take = total_new - pos2;
				Chunk* nc = new Chunk;
				for (int p = 0; p < take; p++) {
					int gi = pos2 + p;
					if (gi < cur_offset)          nc->items[p] = std::move(cur->items[cur_src_offset + gi]);
					else if (gi < cur_offset + n) nc->items[p] = buf[gi - cur_offset];
					else                          nc->items[p] = std::move(cur->items[cur_src_offset + (gi - n)]);
				}
				nc->live = take;
				ncs[ci] = nc;
				pos2 += take;
			}

			current->current_total += n;
			replace_chunk_in_leaf(dp, k, ncs, num_chunks);
			current->cache_leaf = nullptr;
		}

	cleanup:
		trees.erase(src_name);
		delete src_tree;
	}

	inline uint32_t gen_id() {
		return id_total++;
	}

}