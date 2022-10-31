// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 0

island problem: given a 2d grid of 0s and 1s, find the number of islands.

vector<bool> grid;
struct v2 { size_t x; size_t y; };

size_t LinearIdx(v2 dim, v2 pos)
{
  return dim.x * pos.y + pos.x;
}

void DFS(vector<bool>& grid, v2 dim, vector<bool>& visited, v2 pos_start)
{
  vector<v2> stack;
  {
    stack.push_back(pos_start);
    visited[LinearIdx(dim, pos_start)] = 1;
  }
  while (stack.size()) {
    auto pos = stack.back();
    stack.pop_back();
    auto idx = LinearIdx(dim, pos);
    // no need to continue DFS on cells with value=0, they don't form part of the island.
    auto value = grid[idx];
    if (!value) continue;

    // TODO: assert that we haven't visited these before.
    if (pos.x) {
      auto next_pos = pos;
      next_pos.x -= 1;
      stack.push_back(next_pos);
      visited[LinearIdx(dim, next_pos)] = 1;
    }
    if (pos.x + 1 < dim.x) {
      auto next_pos = pos;
      next_pos.x += 1;
      stack.push_back(next_pos);
      visited[LinearIdx(dim, next_pos)] = 1;
    }
    if (pos.y) {
      auto next_pos = pos;
      next_pos.y -= 1;
      stack.push_back(next_pos);
      visited[LinearIdx(dim, next_pos)] = 1;
    }
    if (pos.y + 1 < dim.y) {
      auto next_pos = pos;
      next_pos.y += 1;
      stack.push_back(next_pos);
      visited[LinearIdx(dim, next_pos)] = 1;
    }
  }
}

size_t NumberOfIslands(vector<bool>& grid, v2 dim)
{
  // Equivalent of the DFS-Comp algorithm, which runs a DFS on every unvisited node.
  // If one DFS visits some nodes, we won't revisit those.

  size_t num_islands = 0;
  vector<bool> visited(false, dim.x * dim.y);
  for (size_t y = 0; y < dim.y; ++y) {
    for (size_t x = 0; x < dim.x; ++x) {
      auto idx = LinearIdx(dim.x, x, y);
      auto value = grid[idx];
      // no need to trigger a DFS on cells with value=0
      if (!value) continue;

      auto visited_by_dfs_already = visited[idx];
      if (visited_by_dfs_already) continue;

      // Every time we trigger another DFS, we know we're starting with a new unique graph component.
      // So bump the island count. We could also make the DFS return the component list somehow if
      // we ever wanted to.
      DFS(grid, dim, visited);
      num_islands += 1;
    }
  }
  return num_islands;
}

#endif
