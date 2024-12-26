#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <queue>
#include <cmath>
#include <string>

using namespace std;

// js for printing the total distance
bool ends_with(const string& str, const string& suffix) {
    if (str.size() < suffix.size()) {
        return false;
    }
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}


struct Cell {
    bool isCheckpoint = false;
    bool isEndCheckpoint = false;
    bool isRobotEnd = false;
};

enum ToolMode {
    PLACE_CHECKPOINT,
    PLACE_END_CHECKPOINT,
    PLACE_WALL,
    PLACE_ROBOT_START,
    PLACE_ROBOT_END,
    NONE
};

enum RobotOrientation {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

enum PositionType {
    CENTER,
    MID_TOP,
    MID_RIGHT,
    MID_BOTTOM,
    MID_LEFT,
    CORNER_TOP_LEFT,
    CORNER_TOP_RIGHT,
    CORNER_BOTTOM_LEFT,
    CORNER_BOTTOM_RIGHT
};

struct RobotState {
    int gridX = 0;
    int gridY = 0;
    PositionType positionType = CENTER;
    RobotOrientation orientation = UP;
    bool valid = false;
};

// -----------------------------------------------------------------------------
// Global variables
int gSize = 4;
int CELL_SIZE = 80;
int SIDE_PANEL_WIDTH = 200;
int WINDOW_WIDTH;
int WINDOW_HEIGHT;

vector<vector<Cell>> grid;
vector<vector<bool>> verticalWalls;
vector<vector<bool>> horizontalWalls;

RobotState robotStartState;
RobotState robotEndState;

bool robotStartSet = false;
bool robotEndSet = false;

vector<pair<int,int>> checkpoints;
pair<int,int> endCheckpoint(-1, -1);

ToolMode currentMode = NONE;

struct Button {
    sf::RectangleShape shape;
    string label;
    ToolMode mode;
};

vector<Button> buttons;  
Button findPathButton;        

sf::Font font;
bool fontLoaded = false;

// -----------------------------------------------------------------------------
// Initialize grid and wall containers
void initGrid(int size) {
    gSize = size;
    WINDOW_WIDTH  = gSize * CELL_SIZE + SIDE_PANEL_WIDTH;
    WINDOW_HEIGHT = gSize * CELL_SIZE;

    grid.assign(gSize, vector<Cell>(gSize));
    verticalWalls.assign(gSize-1, vector<bool>(gSize, false));
    horizontalWalls.assign(gSize, vector<bool>(gSize-1, false));
}

// -----------------------------------------------------------------------------
// Initialize sidebar buttons
void initButtons() {
    buttons.clear();
    float x = gSize * CELL_SIZE;
    float y = 0.f;
    float btnHeight = 40.f;
    float margin = 10.f;

    {
        Button b;
        b.shape = sf::RectangleShape(sf::Vector2f(SIDE_PANEL_WIDTH - 2*margin, btnHeight));
        b.shape.setPosition(x + margin, y + margin);
        b.shape.setFillColor(sf::Color(200, 200, 200));
        b.label = "Place Checkpoint";
        b.mode = PLACE_CHECKPOINT;
        buttons.push_back(b);
        y += (btnHeight + margin);
    }

    {
        Button b;
        b.shape = sf::RectangleShape(sf::Vector2f(SIDE_PANEL_WIDTH - 2*margin, btnHeight));
        b.shape.setPosition(x + margin, y + margin);
        b.shape.setFillColor(sf::Color(200, 200, 200));
        b.label = "End Checkpoint";
        b.mode = PLACE_END_CHECKPOINT;
        buttons.push_back(b);
        y += (btnHeight + margin);
    }

    {
        Button b;
        b.shape = sf::RectangleShape(sf::Vector2f(SIDE_PANEL_WIDTH - 2*margin, btnHeight));
        b.shape.setPosition(x + margin, y + margin);
        b.shape.setFillColor(sf::Color(200, 200, 200));
        b.label = "Place Wall";
        b.mode = PLACE_WALL;
        buttons.push_back(b);
        y += (btnHeight + margin);
    }

    {
        Button b;
        b.shape = sf::RectangleShape(sf::Vector2f(SIDE_PANEL_WIDTH - 2*margin, btnHeight));
        b.shape.setPosition(x + margin, y + margin);
        b.shape.setFillColor(sf::Color(200, 200, 200));
        b.label = "Robot Start";
        b.mode = PLACE_ROBOT_START;
        buttons.push_back(b);
        y += (btnHeight + margin);
    }

    {
        Button b;
        b.shape = sf::RectangleShape(sf::Vector2f(SIDE_PANEL_WIDTH - 2*margin, btnHeight));
        b.shape.setPosition(x + margin, y + margin);
        b.shape.setFillColor(sf::Color(200, 200, 200));
        b.label = "Robot End";
        b.mode = PLACE_ROBOT_END;
        buttons.push_back(b);
        y += (btnHeight + margin);
    }

    findPathButton.shape = sf::RectangleShape(sf::Vector2f(SIDE_PANEL_WIDTH - 2*margin, btnHeight));
    findPathButton.shape.setPosition(x + margin, y + margin);
    findPathButton.shape.setFillColor(sf::Color(100, 200, 100));
    findPathButton.label = "Find Path";
    findPathButton.mode = NONE;
}

// -----------------------------------------------------------------------------
// Update button highlights
void updateButtonColors() {
    for (auto &b : buttons) {
        if (b.mode == currentMode) {
            b.shape.setFillColor(sf::Color(150, 150, 250));
        } else {
            b.shape.setFillColor(sf::Color(200, 200, 200));
        }
    }
}

// -----------------------------------------------------------------------------
void setMode(ToolMode mode) {
    currentMode = mode;
    updateButtonColors();
}

// -----------------------------------------------------------------------------
// Check if there's a wall between two adjacent cells
bool isWallBetween(int x1, int y1, int x2, int y2) {
    // Make sure we are not out of range
    if (x2 < 0 || x2 >= gSize || y2 < 0 || y2 >= gSize) {
        // Out of grid => treat as blocked
        return true;
    }

    if (x1 == x2) {
        if (y1 == y2 + 1 && y2 >= 0) {
            return horizontalWalls[x1][y2];
        }
        else if (y1 + 1 == y2 && y1 >= 0) {
            return horizontalWalls[x1][y1];
        }
    }
    else if (y1 == y2) {
        if (x1 == x2 + 1 && x2 >= 0) {
            return verticalWalls[x2][y1];
        }
        else if (x1 + 1 == x2 && x1 >= 0) {
            return verticalWalls[x1][y1];
        }
    }
    // If not strictly adjacent in x or y, treat it as blocked:
    return true;
}

// -----------------------------------------------------------------------------
// Return neighbors (center-to-center) ignoring orientation, but checking walls.
vector<pair<int,int>> getNeighborsIgnoreOrientation(int cx, int cy) {
    vector<pair<int,int>> neighbors;

    // Up
    if (cy > 0 && !isWallBetween(cx, cy, cx, cy - 1)) {
        neighbors.push_back({cx, cy - 1});
    }
    // Down
    if (cy < gSize - 1 && !isWallBetween(cx, cy, cx, cy + 1)) {
        neighbors.push_back({cx, cy + 1});
    }
    // Left
    if (cx > 0 && !isWallBetween(cx, cy, cx - 1, cy)) {
        neighbors.push_back({cx - 1, cy});
    }
    // Right
    if (cx < gSize - 1 && !isWallBetween(cx, cy, cx + 1, cy)) {
        neighbors.push_back({cx + 1, cy});
    }

    return neighbors;
}

// -----------------------------------------------------------------------------
// BFS (center-to-center) ignoring orientation
vector<pair<int,int>> shortestPathBetween(
    pair<int,int> start, 
    pair<int,int> goal, 
    RobotOrientation
) {
    if (start == goal) {
        return { start };
    }

    vector<vector<bool>> visited(gSize, vector<bool>(gSize, false));
    vector<vector<pair<int,int>>> parent(gSize, vector<pair<int,int>>(gSize, {-1, -1}));

    queue<pair<int,int>> q;
    q.push(start);
    visited[start.second][start.first] = true;

    bool found = false;

    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();

        if (cx == goal.first && cy == goal.second) {
            found = true;
            break;
        }

        auto nbrs = getNeighborsIgnoreOrientation(cx, cy);
        for (auto &n : nbrs) {
            int nx = n.first;
            int ny = n.second;
            if (!visited[ny][nx]) {
                visited[ny][nx] = true;
                parent[ny][nx] = {cx, cy};
                q.push({nx, ny});
            }
        }
    }

    if (!found) {
        return {};
    }

    // Reconstruct path
    vector<pair<int,int>> path;
    auto cur = goal;
    while (!(cur.first == -1 && cur.second == -1)) {
        path.push_back(cur);
        cur = parent[cur.second][cur.first];
    }
    reverse(path.begin(), path.end());
    return path;
}

// -----------------------------------------------------------------------------
// Concatenate multiple BFS sub-paths (avoiding duplication of repeated points)
vector<pair<int,int>> concatPaths(const vector<vector<pair<int,int>>>& paths) {
    vector<pair<int,int>> result;
    for (size_t i = 0; i < paths.size(); i++) {
        if (i == 0) {
            result = paths[i];
        }
        else {
            for (size_t j = 1; j < paths[i].size(); j++) {
                result.push_back(paths[i][j]);
            }
        }
    }
    return result;
}

// -----------------------------------------------------------------------------
// Simple path length (count of edges)
double pathLength(const vector<pair<int,int>>& path) {
    if (path.size() < 2) return 0;
    return static_cast<double>(path.size() - 1);
}

// -----------------------------------------------------------------------------
// Structure for BFS permutations among checkpoints
struct PermResult {
    double dist;
    vector<pair<int,int>> finalPath;
};

// -----------------------------------------------------------------------------
// all permutations of checkpoints, always finishing with the special end-checkpoint,
// then finally going to robot end
PermResult findBestPermutation() {
    PermResult best;
    best.dist = numeric_limits<double>::infinity();

    // Copy the normal checkpoints (excluding end checkpoint)
    vector<pair<int,int>> cpts = checkpoints;
    cpts.erase(remove(cpts.begin(), cpts.end(), endCheckpoint), cpts.end());

    vector<int> indices(cpts.size());
    for (int i = 0; i < (int)cpts.size(); i++) {
        indices[i] = i;
    }

    pair<int,int> startPos = {robotStartState.gridX, robotStartState.gridY};
    RobotOrientation fixedO = robotStartState.orientation;

    do {
        bool valid = true;
        vector<vector<pair<int,int>>> partialPaths;
        pair<int,int> prev = startPos;

        // 1) BFS from Start -> first checkpoint
        if (!cpts.empty()) {
            auto path = shortestPathBetween(prev, cpts[indices[0]], fixedO);
            if (path.empty()) { 
                valid = false; 
            } else {
                partialPaths.push_back(path);
                prev = cpts[indices[0]];
            }
        }

        // 2) BFS among the rest of the normal checkpoints
        for (size_t i = 1; i < indices.size() && valid; i++) {
            auto path = shortestPathBetween(prev, cpts[indices[i]], fixedO);
            if (path.empty()) { 
                valid = false; 
                break; 
            }
            partialPaths.push_back(path);
            prev = cpts[indices[i]];
        }

        // 3) BFS to the "end checkpoint" (special checkpoint)
        if (valid) {
            auto path = shortestPathBetween(prev, endCheckpoint, fixedO);
            if (path.empty()) { 
                valid = false; 
            } else {
                partialPaths.push_back(path);
                prev = endCheckpoint;
            }
        }

        // 4) BFS to the final robot end
        if (valid) {
            auto path = shortestPathBetween(prev, {robotEndState.gridX, robotEndState.gridY}, fixedO);
            if (path.empty()) { 
                valid = false; 
            } else {
                partialPaths.push_back(path);
            }
        }

        if (!valid) continue;

        // Merge all partial BFS paths
        auto fullPath = concatPaths(partialPaths);
        double dist = pathLength(fullPath);
        if (dist < best.dist) {
            best.dist = dist;
            best.finalPath = fullPath;
        }
    } while (next_permutation(indices.begin(), indices.end()));

    return best;
}

// -----------------------------------------------------------------------------
// Move from corner/edge to center (or center to corner/edge)
// checks that half-cell moves don't cross grid boundaries
// or pass through wall.
// -----------------------------------------------------------------------------

// Helper to get the half-step commands from posType -> center
vector<string> partialStepsFromPosTypeToCenter(PositionType posType, RobotOrientation ori) {
    vector<string> cmds;

    switch (ori) {
    case UP:
        switch (posType) {
        case CENTER: break;
        case MID_TOP:            cmds.push_back("backward(0.5)");        break;
        case MID_RIGHT:          cmds.push_back("left(0.5)");            break;
        case MID_BOTTOM:         cmds.push_back("forward(0.5)");         break;
        case MID_LEFT:           cmds.push_back("right(0.5)");           break;
        case CORNER_TOP_LEFT:    
            cmds.push_back("right(0.5)");
            cmds.push_back("backward(0.5)");
            break;
        case CORNER_TOP_RIGHT:
            cmds.push_back("left(0.5)");
            cmds.push_back("backward(0.5)");
            break;
        case CORNER_BOTTOM_LEFT:
            cmds.push_back("right(0.5)");
            cmds.push_back("forward(0.5)");
            break;
        case CORNER_BOTTOM_RIGHT:
            cmds.push_back("left(0.5)");
            cmds.push_back("forward(0.5)");
            break;
        }
        break;

    case DOWN:
        switch (posType) {
        case CENTER: break;
        case MID_TOP:            cmds.push_back("forward(0.5)");         break;
        case MID_RIGHT:          cmds.push_back("right(0.5)");           break;
        case MID_BOTTOM:         cmds.push_back("backward(0.5)");        break;
        case MID_LEFT:           cmds.push_back("left(0.5)");            break;
        case CORNER_TOP_LEFT:
            cmds.push_back("left(0.5)");
            cmds.push_back("forward(0.5)");
            break;
        case CORNER_TOP_RIGHT:
            cmds.push_back("right(0.5)");
            cmds.push_back("forward(0.5)");
            break;
        case CORNER_BOTTOM_LEFT:
            cmds.push_back("left(0.5)");
            cmds.push_back("backward(0.5)");
            break;
        case CORNER_BOTTOM_RIGHT:
            cmds.push_back("right(0.5)");
            cmds.push_back("backward(0.5)");
            break;
        }
        break;

    case LEFT:
        switch (posType) {
        case CENTER: break;
        case MID_TOP:            cmds.push_back("right(0.5)");           break;
        case MID_RIGHT:          cmds.push_back("backward(0.5)");        break;
        case MID_BOTTOM:         cmds.push_back("left(0.5)");            break;
        case MID_LEFT:           cmds.push_back("forward(0.5)");         break;
        case CORNER_TOP_LEFT:
            cmds.push_back("forward(0.5)");
            cmds.push_back("right(0.5)");
            break;
        case CORNER_TOP_RIGHT:
            cmds.push_back("backward(0.5)");
            cmds.push_back("right(0.5)");
            break;
        case CORNER_BOTTOM_LEFT:
            cmds.push_back("forward(0.5)");
            cmds.push_back("left(0.5)");
            break;
        case CORNER_BOTTOM_RIGHT:
            cmds.push_back("backward(0.5)");
            cmds.push_back("left(0.5)");
            break;
        }
        break;

    case RIGHT:
        switch (posType) {
        case CENTER: break;
        case MID_TOP:            cmds.push_back("left(0.5)");            break;
        case MID_RIGHT:          cmds.push_back("forward(0.5)");         break;
        case MID_BOTTOM:         cmds.push_back("right(0.5)");           break;
        case MID_LEFT:           cmds.push_back("backward(0.5)");        break;
        case CORNER_TOP_LEFT:
            cmds.push_back("backward(0.5)");
            cmds.push_back("left(0.5)");
            break;
        case CORNER_TOP_RIGHT:
            cmds.push_back("forward(0.5)");
            cmds.push_back("left(0.5)");
            break;
        case CORNER_BOTTOM_LEFT:
            cmds.push_back("backward(0.5)");
            cmds.push_back("right(0.5)");
            break;
        case CORNER_BOTTOM_RIGHT:
            cmds.push_back("forward(0.5)");
            cmds.push_back("right(0.5)");
            break;
        }
        break;
    }

    return cmds;
}

// Helper to get the half-step commands from center -> posType
vector<string> partialStepsFromCenterToPosType(PositionType posType, RobotOrientation ori) {
    vector<string> cmds;

    switch (ori) {
    case UP:
        switch (posType) {
        case CENTER: break;
        case MID_TOP:            cmds.push_back("forward(0.5)");         break;
        case MID_RIGHT:          cmds.push_back("right(0.5)");           break;
        case MID_BOTTOM:         cmds.push_back("backward(0.5)");        break;
        case MID_LEFT:           cmds.push_back("left(0.5)");            break;
        case CORNER_TOP_LEFT:    
            cmds.push_back("backward(0.5)");
            cmds.push_back("left(0.5)");
            break;
        case CORNER_TOP_RIGHT:
            cmds.push_back("backward(0.5)");
            cmds.push_back("right(0.5)");
            break;
        case CORNER_BOTTOM_LEFT:
            cmds.push_back("forward(0.5)");
            cmds.push_back("left(0.5)");
            break;
        case CORNER_BOTTOM_RIGHT:
            cmds.push_back("forward(0.5)");
            cmds.push_back("right(0.5)");
            break;
        }
        break;

    case DOWN:
        switch (posType) {
        case CENTER: break;
        case MID_TOP:            cmds.push_back("backward(0.5)");        break;
        case MID_RIGHT:          cmds.push_back("left(0.5)");            break;
        case MID_BOTTOM:         cmds.push_back("forward(0.5)");         break;
        case MID_LEFT:           cmds.push_back("right(0.5)");           break;
        case CORNER_TOP_LEFT:
            cmds.push_back("backward(0.5)");
            cmds.push_back("left(0.5)");
            break;
        case CORNER_TOP_RIGHT:
            cmds.push_back("backward(0.5)");
            cmds.push_back("right(0.5)");
            break;
        case CORNER_BOTTOM_LEFT:
            cmds.push_back("forward(0.5)");
            cmds.push_back("left(0.5)");
            break;
        case CORNER_BOTTOM_RIGHT:
            cmds.push_back("forward(0.5)");
            cmds.push_back("right(0.5)");
            break;
        }
        break;

    case LEFT:
        switch (posType) {
        case CENTER: break;
        case MID_TOP:            cmds.push_back("left(0.5)");            break;
        case MID_RIGHT:          cmds.push_back("forward(0.5)");         break;
        case MID_BOTTOM:         cmds.push_back("right(0.5)");           break;
        case MID_LEFT:           cmds.push_back("backward(0.5)");        break;
        case CORNER_TOP_LEFT:
            cmds.push_back("right(0.5)");
            cmds.push_back("forward(0.5)");
            break;
        case CORNER_TOP_RIGHT:
            cmds.push_back("right(0.5)");
            cmds.push_back("backward(0.5)");
            break;
        case CORNER_BOTTOM_LEFT:
            cmds.push_back("left(0.5)");
            cmds.push_back("forward(0.5)");
            break;
        case CORNER_BOTTOM_RIGHT:
            cmds.push_back("left(0.5)");
            cmds.push_back("backward(0.5)");
            break;
        }
        break;

    case RIGHT:
        switch (posType) {
        case CENTER: break;
        case MID_TOP:            cmds.push_back("right(0.5)");           break;
        case MID_RIGHT:          cmds.push_back("backward(0.5)");        break;
        case MID_BOTTOM:         cmds.push_back("left(0.5)");            break;
        case MID_LEFT:           cmds.push_back("forward(0.5)");         break;
        case CORNER_TOP_LEFT:
            cmds.push_back("left(0.5)");
            cmds.push_back("backward(0.5)");
            break;
        case CORNER_TOP_RIGHT:
            cmds.push_back("left(0.5)");
            cmds.push_back("forward(0.5)");
            break;
        case CORNER_BOTTOM_LEFT:
            cmds.push_back("right(0.5)");
            cmds.push_back("backward(0.5)");
            break;
        case CORNER_BOTTOM_RIGHT:
            cmds.push_back("right(0.5)");
            cmds.push_back("forward(0.5)");
            break;
        }
        break;
    }

    return cmds;
}

// -----------------------------------------------------------------------------
// allow the robot to start on the very edge by focusing on cell
// indices rather than raw float positions. fail if  cell index
// invalid
// -----------------------------------------------------------------------------

bool canDoPartialStepsToCenter(int x, int y, PositionType posType, RobotOrientation ori) {
    vector<string> stepCmds = partialStepsFromPosTypeToCenter(posType, ori);

    double currentX = (double)x; 
    double currentY = (double)y;

    auto interpretMove = [&](const string& cmd) {
        double dist = 0.0;
        auto openParenPos = cmd.find('(');
        auto closeParenPos = cmd.find(')');
        if (openParenPos == string::npos || closeParenPos == string::npos) {
            return false;  // malformed
        }
        string dir = cmd.substr(0, openParenPos);
        string val = cmd.substr(openParenPos+1, closeParenPos - (openParenPos+1));
        dist = stod(val);

        // Apply move in grid coords
        if (ori == UP) {
            if (dir == "forward")    currentY -= dist;
            else if (dir == "backward") currentY += dist;
            else if (dir == "left")     currentX -= dist;
            else if (dir == "right")    currentX += dist;
        }
        else if (ori == DOWN) {
            if (dir == "forward")    currentY += dist;
            else if (dir == "backward") currentY -= dist;
            else if (dir == "left")     currentX += dist;
            else if (dir == "right")    currentX -= dist;
        }
        else if (ori == LEFT) {
            if (dir == "forward")    currentX -= dist;
            else if (dir == "backward") currentX += dist;
            else if (dir == "left")     currentY += dist;
            else if (dir == "right")    currentY -= dist;
        }
        else if (ori == RIGHT) {
            if (dir == "forward")    currentX += dist;
            else if (dir == "backward") currentX -= dist;
            else if (dir == "left")     currentY -= dist;
            else if (dir == "right")    currentY += dist;
        }

        // Only fail if the integer cell index goes fully out of range
        int cellX = (int)floor(currentX + 0.0001);
        int cellY = (int)floor(currentY + 0.0001);
        if (cellX < 0 || cellX >= gSize || cellY < 0 || cellY >= gSize) {
            return false;
        }
        return true;
    };

    for (auto &c : stepCmds) {
        if (!interpretMove(c)) {
            return false;
        }
    }

    return true;
}

// Same fix for center -> posType
bool canDoPartialStepsFromCenter(int x, int y, PositionType posType, RobotOrientation ori) {
    vector<string> stepCmds = partialStepsFromCenterToPosType(posType, ori);

    double currentX = (double)x;
    double currentY = (double)y;

    auto interpretMove = [&](const string& cmd) {
        double dist = 0.0;
        auto openParenPos = cmd.find('(');
        auto closeParenPos = cmd.find(')');
        if (openParenPos == string::npos || closeParenPos == string::npos) {
            return false; 
        }
        string dir = cmd.substr(0, openParenPos);
        string val = cmd.substr(openParenPos+1, closeParenPos - (openParenPos+1));
        dist = stod(val);

        if (ori == UP) {
            if (dir == "forward")    currentY -= dist;
            else if (dir == "backward") currentY += dist;
            else if (dir == "left")     currentX -= dist;
            else if (dir == "right")    currentX += dist;
        }
        else if (ori == DOWN) {
            if (dir == "forward")    currentY += dist;
            else if (dir == "backward") currentY -= dist;
            else if (dir == "left")     currentX += dist;
            else if (dir == "right")    currentX -= dist;
        }
        else if (ori == LEFT) {
            if (dir == "forward")    currentX -= dist;
            else if (dir == "backward") currentX += dist;
            else if (dir == "left")     currentY += dist;
            else if (dir == "right")    currentY -= dist;
        }
        else if (ori == RIGHT) {
            if (dir == "forward")    currentX += dist;
            else if (dir == "backward") currentX -= dist;
            else if (dir == "left")     currentY -= dist;
            else if (dir == "right")    currentY += dist;
        }

        int cellX = (int)floor(currentX + 0.0001);
        int cellY = (int)floor(currentY + 0.0001);
        if (cellX < 0 || cellX >= gSize || cellY < 0 || cellY >= gSize) {
            return false;
        }
        return true;
    };

    for (auto &c : stepCmds) {
        if (!interpretMove(c)) {
            return false;
        }
    }

    return true;
}

// -----------------------------------------------------------------------------
// Convert  final BFS path from center to center into the robot commands
// plus partial steps at start and end.
vector<string> pathToCommands(const vector<pair<int,int>>& path, RobotOrientation ori) {
    if (path.empty()) {
        return {};
    }

    vector<string> commands;

    // 1) Partial steps from actual start positionType to the cell center
    vector<string> prefix = partialStepsFromPosTypeToCenter(robotStartState.positionType, ori);
    commands.insert(commands.end(), prefix.begin(), prefix.end());

    // 2) Convert BFS center-to-center path into movement commands
    for (size_t i = 0; i + 1 < path.size(); i++) {
        auto [px, py] = path[i];
        auto [nx, ny] = path[i+1];

        int dx = nx - px;
        int dy = ny - py;

        if (ori == UP) {
            if (dx == 0 && dy == -1) commands.push_back("forward(1)");
            else if (dx == 0 && dy == 1) commands.push_back("backward(1)");
            else if (dx == -1 && dy == 0) commands.push_back("left(1)");
            else if (dx == 1 && dy == 0) commands.push_back("right(1)");
        }
        else if (ori == DOWN) {
            if (dx == 0 && dy == 1) commands.push_back("forward(1)");
            else if (dx == 0 && dy == -1) commands.push_back("backward(1)");
            else if (dx == 1 && dy == 0) commands.push_back("left(1)");
            else if (dx == -1 && dy == 0) commands.push_back("right(1)");
        }
        else if (ori == LEFT) {
            if (dx == -1 && dy == 0) commands.push_back("forward(1)");
            else if (dx == 1 && dy == 0) commands.push_back("backward(1)");
            else if (dx == 0 && dy == 1) commands.push_back("left(1)");
            else if (dx == 0 && dy == -1) commands.push_back("right(1)");
        }
        else if (ori == RIGHT) {
            if (dx == 1 && dy == 0) commands.push_back("forward(1)");
            else if (dx == -1 && dy == 0) commands.push_back("backward(1)");
            else if (dx == 0 && dy == -1) commands.push_back("left(1)");
            else if (dx == 0 && dy == 1) commands.push_back("right(1)");
        }
    }

    // 3) Partial steps from the center to the end positionType
    vector<string> suffix = partialStepsFromCenterToPosType(robotEndState.positionType, ori);
    commands.insert(commands.end(), suffix.begin(), suffix.end());

    return commands;
}

// -----------------------------------------------------------------------------
// Handle clicks on side panel
void handleSidePanelClick(int mx, int my) {
    for (auto &b : buttons) {
        if (b.shape.getGlobalBounds().contains(mx, my)) {
            setMode(b.mode);
            return;
        }
    }

    if (findPathButton.shape.getGlobalBounds().contains(mx, my)) {
        if (!robotStartSet || !robotEndSet || endCheckpoint.first < 0) {
            cout << "Not all conditions met (start/end or end checkpoint not set).\n";
            return;
        }

        // Check partial steps from start corner/edge => center
        if (!canDoPartialStepsToCenter(robotStartState.gridX, robotStartState.gridY,
                                       robotStartState.positionType,
                                       robotStartState.orientation))
        {
            cout << "Cannot move from start corner/edge to center without going off-grid.\n";
            cout << "No path found.\n";
            return;
        }

        // BFS permutations among checkpoints
        PermResult best = findBestPermutation();
        if (best.finalPath.empty()) {
            cout << "No path found.\n";
            return;
        }

        // Check partial steps from final cell center => end corner/edge
        auto [ex, ey] = best.finalPath.back(); 
        if (!canDoPartialStepsFromCenter(ex, ey,
                                         robotEndState.positionType,
                                         robotStartState.orientation))
        {
            cout << "Cannot move from center to final corner/edge without going off-grid.\n";
            cout << "No path found.\n";
            return;
        }

        // If we made it here, we have a valid path
        vector<string> commands = pathToCommands(best.finalPath, robotStartState.orientation);
        if (commands.empty()) {
            cout << "No path found.\n";
        } else {
            float totaldistance = 0;
            
            for (const string &c : commands) {
                if(ends_with(c, "(0.5)")){
                    totaldistance += 0.5;
                } else{
                    totaldistance += 1;
                }
                cout << c << "\n";
            }
            cout << "total distance: " << totaldistance << "\n";
        }
        exit(0);
    }
}

// -----------------------------------------------------------------------------
// Handle clicks on the grid
void handleGridClick(int mx, int my) {
    int gx = mx / CELL_SIZE;
    int gy = my / CELL_SIZE;
    if (gx < 0 || gy < 0 || gx >= gSize || gy >= gSize) return;

    int localX = mx % CELL_SIZE;
    int localY = my % CELL_SIZE;

    PositionType posType = CENTER;
    int threshold = CELL_SIZE / 4;

    // Determine corner/edge portion
    if (localX < threshold && localY < threshold) {
        posType = CORNER_TOP_LEFT;
    }
    else if (localX > CELL_SIZE - threshold && localY < threshold) {
        posType = CORNER_TOP_RIGHT;
    }
    else if (localX < threshold && localY > CELL_SIZE - threshold) {
        posType = CORNER_BOTTOM_LEFT;
    }
    else if (localX > CELL_SIZE - threshold && localY > CELL_SIZE - threshold) {
        posType = CORNER_BOTTOM_RIGHT;
    }
    else if (localY < threshold) {
        posType = MID_TOP;
    }
    else if (localX > CELL_SIZE - threshold) {
        posType = MID_RIGHT;
    }
    else if (localY > CELL_SIZE - threshold) {
        posType = MID_BOTTOM;
    }
    else if (localX < threshold) {
        posType = MID_LEFT;
    }

    if (currentMode == PLACE_WALL) {
        bool placedWall = false;
        // Vertical walls
        if (localX < threshold && gx > 0) {
            verticalWalls[gx-1][gy] = !verticalWalls[gx-1][gy];
            placedWall = true;
        }
        else if (localX > CELL_SIZE - threshold && gx < gSize - 1) {
            verticalWalls[gx][gy] = !verticalWalls[gx][gy];
            placedWall = true;
        }
        // Horizontal walls
        else if (localY < threshold && gy > 0) {
            horizontalWalls[gx][gy-1] = !horizontalWalls[gx][gy-1];
            placedWall = true;
        }
        else if (localY > CELL_SIZE - threshold && gy < gSize - 1) {
            horizontalWalls[gx][gy] = !horizontalWalls[gx][gy];
            placedWall = true;
        }
        if (!placedWall) {
            cout << "Click near a cell edge to place/remove a wall.\n";
        }
        return;
    }

    switch (currentMode) {
    case PLACE_CHECKPOINT: {
        bool wasCheckpoint = grid[gy][gx].isCheckpoint;
        // If we toggle a checkpoint that was an end checkpoint, remove end checkpoint status
        if ((gx == endCheckpoint.first) && (gy == endCheckpoint.second)) {
            grid[gy][gx].isEndCheckpoint = false;
            endCheckpoint = {-1, -1};
        }

        // Toggle normal checkpoint
        grid[gy][gx].isCheckpoint = !wasCheckpoint;
        if (grid[gy][gx].isCheckpoint) {
            checkpoints.emplace_back(gx, gy);
        }
        else {
            checkpoints.erase(remove(checkpoints.begin(), checkpoints.end(),
                                          make_pair(gx, gy)), checkpoints.end());
        }
    } break;

    case PLACE_END_CHECKPOINT: {
        // Remove the old end checkpoint if any
        if (endCheckpoint.first >= 0) {
            grid[endCheckpoint.second][endCheckpoint.first].isEndCheckpoint = false;
        }
        grid[gy][gx].isEndCheckpoint = true;
        endCheckpoint = {gx, gy};
        // Also remove it from normal checkpoint list if present
        checkpoints.erase(remove(checkpoints.begin(), checkpoints.end(), endCheckpoint),
                          checkpoints.end());
    } break;

    case PLACE_ROBOT_START: {
        robotStartState.gridX = gx;
        robotStartState.gridY = gy;
        robotStartState.positionType = posType;
        robotStartState.valid = true;
        robotStartSet = true;
    } break;

    case PLACE_ROBOT_END: {
        if (robotEndState.valid) {
            grid[robotEndState.gridY][robotEndState.gridX].isRobotEnd = false;
        }
        robotEndState.gridX = gx;
        robotEndState.gridY = gy;
        robotEndState.positionType = posType;
        robotEndState.valid = true;
        grid[gy][gx].isRobotEnd = true;
        robotEndSet = true;
    } break;

    default:
        break;
    }
}

// -----------------------------------------------------------------------------
int main() {
    cout << "Enter grid size (e.g., 4, 5, etc.): ";
    cin >> gSize;
    if (gSize < 2) {
        cout << "Invalid grid size. Defaulting to 4.\n";
        gSize = 4;
    }

    initGrid(gSize);

    cout << "Enter robot starting orientation (up, right, down, left): ";
    string orientInput;
    cin >> orientInput;
    transform(orientInput.begin(), orientInput.end(), orientInput.begin(), ::tolower);
    if (orientInput == "up") {
        robotStartState.orientation = UP;
    }
    else if (orientInput == "right") {
        robotStartState.orientation = RIGHT;
    }
    else if (orientInput == "down") {
        robotStartState.orientation = DOWN;
    }
    else if (orientInput == "left") {
        robotStartState.orientation = LEFT;
    }
    else {
        cout << "Invalid orientation. Defaulting to UP.\n";
        robotStartState.orientation = UP;
    }

    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Robot Tour GUI");
    window.setFramerateLimit(60);

    if (!font.loadFromFile("arial.ttf")) {
        cout << "Warning: Failed to load arial.ttf. No text will be displayed.\n";
    }
    else {
        fontLoaded = true;
    }

    initButtons();
    updateButtonColors();

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    int mx = event.mouseButton.x;
                    int my = event.mouseButton.y;
                    if (mx > gSize * CELL_SIZE) {
                        handleSidePanelClick(mx, my);
                    }
                    else {
                        if (currentMode != NONE) {
                            handleGridClick(mx, my);
                        }
                    }
                }
            }
        }

        window.clear(sf::Color::White);

        // Draw grid cells
        for (int y = 0; y < gSize; y++) {
            for (int x = 0; x < gSize; x++) {
                sf::RectangleShape cellRect(sf::Vector2f(CELL_SIZE, CELL_SIZE));
                cellRect.setPosition(static_cast<float>(x) * CELL_SIZE,
                                     static_cast<float>(y) * CELL_SIZE);
                cellRect.setOutlineThickness(1.f);
                cellRect.setOutlineColor(sf::Color::Black);

                if (grid[y][x].isCheckpoint) {
                    cellRect.setFillColor(sf::Color::Green);
                }
                else if (grid[y][x].isEndCheckpoint) {
                    cellRect.setFillColor(sf::Color::Cyan);
                }
                else {
                    cellRect.setFillColor(sf::Color::White);
                }
                window.draw(cellRect);
            }
        }

        // Draw vertical walls
        for (int x = 0; x < gSize - 1; x++) {
            for (int y = 0; y < gSize; y++) {
                if (verticalWalls[x][y]) {
                    sf::RectangleShape wall(sf::Vector2f(4, static_cast<float>(CELL_SIZE)));
                    wall.setPosition(static_cast<float>((x + 1) * CELL_SIZE - 2),
                                     static_cast<float>(y * CELL_SIZE));
                    wall.setFillColor(sf::Color::Black);
                    window.draw(wall);
                }
            }
        }

        // Draw horizontal walls
        for (int x = 0; x < gSize; x++) {
            for (int y = 0; y < gSize - 1; y++) {
                if (horizontalWalls[x][y]) {
                    sf::RectangleShape wall(sf::Vector2f(static_cast<float>(CELL_SIZE), 4.f));
                    wall.setPosition(static_cast<float>(x * CELL_SIZE),
                                     static_cast<float>((y + 1) * CELL_SIZE - 2));
                    wall.setFillColor(sf::Color::Black);
                    window.draw(wall);
                }
            }
        }

        // Draw robot end if set
        if (robotEndSet) {
            sf::CircleShape endCircle(10.f);
            float rx = robotEndState.gridX * CELL_SIZE;
            float ry = robotEndState.gridY * CELL_SIZE;

            switch (robotEndState.positionType) {
            case CENTER:
                rx += CELL_SIZE / 2.f;
                ry += CELL_SIZE / 2.f;
                break;
            case MID_TOP:
                rx += CELL_SIZE / 2.f;
                break;
            case MID_RIGHT:
                rx += CELL_SIZE;
                ry += CELL_SIZE / 2.f;
                break;
            case MID_BOTTOM:
                rx += CELL_SIZE / 2.f;
                ry += CELL_SIZE;
                break;
            case MID_LEFT:
                ry += CELL_SIZE / 2.f;
                break;
            case CORNER_TOP_LEFT:
                break;
            case CORNER_TOP_RIGHT:
                rx += CELL_SIZE;
                break;
            case CORNER_BOTTOM_LEFT:
                ry += CELL_SIZE;
                break;
            case CORNER_BOTTOM_RIGHT:
                rx += CELL_SIZE;
                ry += CELL_SIZE;
                break;
            }

            endCircle.setFillColor(sf::Color::Red);
            endCircle.setPosition(rx - 10.f, ry - 10.f);
            window.draw(endCircle);
        }

        // Draw robot start if set
        if (robotStartSet) {
            sf::CircleShape robot(12.f);
            float rx = robotStartState.gridX * CELL_SIZE;
            float ry = robotStartState.gridY * CELL_SIZE;

            switch (robotStartState.positionType) {
            case CENTER:
                rx += CELL_SIZE / 2.f;
                ry += CELL_SIZE / 2.f;
                break;
            case MID_TOP:
                rx += CELL_SIZE / 2.f;
                break;
            case MID_RIGHT:
                rx += CELL_SIZE;
                ry += CELL_SIZE / 2.f;
                break;
            case MID_BOTTOM:
                rx += CELL_SIZE / 2.f;
                ry += CELL_SIZE;
                break;
            case MID_LEFT:
                ry += CELL_SIZE / 2.f;
                break;
            case CORNER_TOP_LEFT:
                break;
            case CORNER_TOP_RIGHT:
                rx += CELL_SIZE;
                break;
            case CORNER_BOTTOM_LEFT:
                ry += CELL_SIZE;
                break;
            case CORNER_BOTTOM_RIGHT:
                rx += CELL_SIZE;
                ry += CELL_SIZE;
                break;
            }

            robot.setFillColor(sf::Color::Blue);
            robot.setPosition(rx - 12.f, ry - 12.f);
            window.draw(robot);

            // Indicate orientation with a small line
            sf::RectangleShape dirLine(sf::Vector2f(20, 2));
            dirLine.setOrigin(0.f, 1.f);
            dirLine.setPosition(rx, ry);
            switch (robotStartState.orientation) {
                case UP:    dirLine.setRotation(270.f); break;
                case RIGHT: dirLine.setRotation(0.f);   break;
                case DOWN:  dirLine.setRotation(90.f);  break;
                case LEFT:  dirLine.setRotation(180.f); break;
            }
            dirLine.setFillColor(sf::Color::Blue);
            window.draw(dirLine);
        }

        // Draw side panel
        sf::RectangleShape panel(sf::Vector2f(static_cast<float>(SIDE_PANEL_WIDTH),
                                              static_cast<float>(WINDOW_HEIGHT)));
        panel.setPosition(static_cast<float>(gSize * CELL_SIZE), 0.f);
        panel.setFillColor(sf::Color(220, 220, 220));
        window.draw(panel);

        // Draw main buttons
        for (auto &b : buttons) {
            window.draw(b.shape);
            if (fontLoaded) {
                sf::Text txt;
                txt.setFont(font);
                txt.setString(b.label);
                txt.setCharacterSize(14);
                txt.setFillColor(sf::Color::Black);
                txt.setPosition(b.shape.getPosition().x + 10, b.shape.getPosition().y + 10);
                window.draw(txt);
            }
        }

        // Draw "Find Path" button
        window.draw(findPathButton.shape);
        if (fontLoaded) {
            sf::Text txt;
            txt.setFont(font);
            txt.setString(findPathButton.label);
            txt.setCharacterSize(14);
            txt.setFillColor(sf::Color::Black);
            txt.setPosition(findPathButton.shape.getPosition().x + 10,
                            findPathButton.shape.getPosition().y + 10);
            window.draw(txt);
        }

        window.display();
    }

    return 0;
}
