#include "ExplorerPanel.h"
#include "GuiManager.h"
#include <raylib.h>
#include <raymath.h>
#include <algorithm>
#include <unordered_map>
#include "bootstrap/Instance.h"
#include "core/logging/Logging.h"

ExplorerPanel::ExplorerPanel(GuiManager* manager) : guiManager(manager) {
}

ExplorerPanel::~ExplorerPanel() {
}

void ExplorerPanel::Render(Rectangle bounds) {
    // Draw panel background
    DrawRectangleRec(bounds, BACKGROUND_COLOR);
    DrawRectangleLinesEx(bounds, 1.0f, {60, 60, 60, 255});
    
    // Draw title bar
    Rectangle titleBar = {bounds.x, bounds.y, bounds.width, 25.0f};
    DrawRectangleRec(titleBar, {40, 40, 40, 255});
    if (GuiManager::IsCustomFontLoaded()) {
        DrawTextEx(GuiManager::GetCustomFont(), "Explorer", {titleBar.x + 8, titleBar.y + 4}, 20, 1.5f, TEXT_COLOR);
    } else {
        DrawText("Explorer", (int)(titleBar.x + 8), (int)(titleBar.y + 4), 20, TEXT_COLOR);
    }
    
    // Draw search bar
    Rectangle searchBar = {bounds.x + 2, bounds.y + 27, bounds.width - 4, 25.0f};
    Color searchBgColor = searchActive ? Color{40, 40, 40, 255} : Color{35, 35, 35, 255};
    DrawRectangleRec(searchBar, searchBgColor);
    Color searchBorderColor = searchActive ? Color{70, 130, 180, 255} : Color{60, 60, 60, 255};
    DrawRectangleLinesEx(searchBar, 1.0f, searchBorderColor);
    
    // Draw search icon
    if (GuiManager::IsCustomFontLoaded()) {
        DrawTextEx(GuiManager::GetCustomFont(), "S", {searchBar.x + 4, searchBar.y + 4}, 16, 1.2f, {150, 150, 150, 255});
    } else {
        DrawText("S", (int)(searchBar.x + 4), (int)(searchBar.y + 4), 16, {150, 150, 150, 255});
    }
    
    // Draw search text
    std::string displayText = searchQuery.empty() ? "Search..." : searchQuery;
    Color textColor = searchQuery.empty() ? Color{100, 100, 100, 255} : TEXT_COLOR;
    
    if (GuiManager::IsCustomFontLoaded()) {
        DrawTextEx(GuiManager::GetCustomFont(), displayText.c_str(), {searchBar.x + 20, searchBar.y + 4}, 16, 1.2f, textColor);
    } else {
        DrawText(displayText.c_str(), (int)(searchBar.x + 20), (int)(searchBar.y + 4), 16, textColor);
    }
    
    // Draw cursor if search is active
    if (searchActive && fmod(searchBlinkTimer, 1.0) < 0.5) {
        int textWidth;
        if (GuiManager::IsCustomFontLoaded()) {
            Vector2 textSize = MeasureTextEx(GuiManager::GetCustomFont(), searchQuery.substr(0, searchCursorPos).c_str(), 16, 1.2f);
            textWidth = (int)textSize.x;
        } else {
            textWidth = MeasureText(searchQuery.substr(0, searchCursorPos).c_str(), 16);
        }
        DrawLine((int)(searchBar.x + 20 + textWidth), (int)(searchBar.y + 2), 
                (int)(searchBar.x + 20 + textWidth), (int)(searchBar.y + searchBar.height - 2), TEXT_COLOR);
    }
    
    // Handle search bar click
    if (CheckCollisionPointRec(GetMousePosition(), searchBar) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        searchActive = true;
        searchCursorPos = (int)searchQuery.length();
    }
    
    // Content area (adjusted for search bar)
    // let's manually set this shall we?
    Rectangle contentArea = {
        bounds.x + 2,
        bounds.y + 54, // 27 (title) + 25 (search) + 2 (padding)
        bounds.width - 4,
        bounds.height - 56 // adjusted for search bar
    };
    
    // Enable scissor test for scrolling
    BeginScissorMode((int)contentArea.x, (int)contentArea.y, (int)contentArea.width, (int)contentArea.height);
    
    float yOffset = contentArea.y - scrollY;
    contentHeight = 0.0f;
    
    if (rootNode) {
        RenderTreeNode(rootNode, contentArea, yOffset);
    }
    
    EndScissorMode();
    
    // Draw scrollbar if needed
    if (contentHeight > contentArea.height) {
        float scrollbarHeight = (contentArea.height / contentHeight) * contentArea.height;
        float scrollbarY = contentArea.y + (scrollY / contentHeight) * contentArea.height;
        
        Rectangle scrollbarBg = {
            contentArea.x + contentArea.width - 8,
            contentArea.y,
            8,
            contentArea.height
        };
        
        Rectangle scrollbarThumb = {
            scrollbarBg.x + 1,
            scrollbarY,
            6,
            scrollbarHeight
        };
        
        DrawRectangleRec(scrollbarBg, {20, 20, 20, 255});
        DrawRectangleRec(scrollbarThumb, {80, 80, 80, 255});
    }
    
    // Render context menu on top of everything
    if (contextMenuActive) {
        RenderContextMenu(bounds);
    }
    
    // Render insert submenu on top of context menu
    if (insertSubmenuActive) {
        RenderInsertSubmenu(bounds);
    }
}

void ExplorerPanel::Update() {
    // Update rename blink timer
    if (renameActive) {
        renameBlinkTimer += GetFrameTime();
    }
    
    // Update search blink timer
    if (searchActive) {
        searchBlinkTimer += GetFrameTime();
        
        // Handle search text input
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 125) {
                searchQuery.insert(searchCursorPos, 1, (char)key);
                searchCursorPos++;
            }
            key = GetCharPressed();
        }
        
        // Handle special keys for search
        if (IsKeyPressed(KEY_BACKSPACE) && searchCursorPos > 0) {
            searchQuery.erase(searchCursorPos - 1, 1);
            searchCursorPos--;
        }
        
        if (IsKeyPressed(KEY_DELETE) && searchCursorPos < (int)searchQuery.length()) {
            searchQuery.erase(searchCursorPos, 1);
        }
        
        if (IsKeyPressed(KEY_LEFT) && searchCursorPos > 0) {
            searchCursorPos--;
        }
        
        if (IsKeyPressed(KEY_RIGHT) && searchCursorPos < (int)searchQuery.length()) {
            searchCursorPos++;
        }
        
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
            searchActive = false;
        }
    } else if (renameActive) {
        // Handle rename text input
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 125) {
                renameText.insert(renameCursorPos, 1, (char)key);
                renameCursorPos++;
            }
            key = GetCharPressed();
        }
        
        // Handle special keys for rename
        if (IsKeyPressed(KEY_BACKSPACE) && renameCursorPos > 0) {
            renameText.erase(renameCursorPos - 1, 1);
            renameCursorPos--;
        }
        
        if (IsKeyPressed(KEY_DELETE) && renameCursorPos < (int)renameText.length()) {
            renameText.erase(renameCursorPos, 1);
        }
        
        if (IsKeyPressed(KEY_LEFT) && renameCursorPos > 0) {
            renameCursorPos--;
        }
        
        if (IsKeyPressed(KEY_RIGHT) && renameCursorPos < (int)renameText.length()) {
            renameCursorPos++;
        }
        
        if (IsKeyPressed(KEY_ENTER)) {
            FinishRename();
        }
        
        if (IsKeyPressed(KEY_ESCAPE)) {
            CancelRename();
        }
    } else {
        // Handle arrow key navigation when search and rename are not active
        if (IsKeyPressed(KEY_UP)) {
            NavigateUp();
        }
        
        if (IsKeyPressed(KEY_DOWN)) {
            NavigateDown();
        }
    }
    
    // Handle mouse clicks
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetMousePosition();
        
        // Check if context menu is active and handle clicks
        if (contextMenuActive || insertSubmenuActive) {
            if (!HandleContextMenuClick(mousePos)) {
                // Click outside context menu - hide it
                HideContextMenu();
            }
        } else if (renameActive) {
            // Finish rename if clicking outside rename area
            FinishRename();
        }
        
        // Deactivate search if clicked outside search bar (handled in Render method)
    }
    
    // Handle scrolling
    Vector2 mousePos = GetMousePosition();
    float wheelMove = GetMouseWheelMove();
    
    if (wheelMove != 0.0f) {
        scrollY -= wheelMove * 20.0f;
        scrollY = Clamp(scrollY, 0.0f, fmaxf(0.0f, contentHeight - 400.0f));
    }
    
    // Auto-refresh tree periodically to catch changes
    static double lastRefreshTime = 0.0;
    double currentTime = GetTime();
    if (currentTime - lastRefreshTime > 1.0) { // Refresh every second
        RefreshTree();
        lastRefreshTime = currentTime;
    }
    
    // Update visible nodes list and selected node index
    BuildVisibleNodesList();
    UpdateSelectedNodeIndex();
}

void ExplorerPanel::RefreshTree() {
    if (!rootInstance) return;
    
    // Store current expansion states before rebuilding
    std::unordered_map<std::string, bool> expansionStates;
    if (rootNode) {
        StoreExpansionStates(rootNode, expansionStates);
    }
    
    rootNode = std::make_shared<TreeNode>();
    rootNode->instance = rootInstance;
    rootNode->expanded = true;
    rootNode->depth = 0;
    
    BuildTreeFromInstance(rootInstance, rootNode, 0);
    
    // Restore expansion states
    if (!expansionStates.empty()) {
        RestoreExpansionStates(rootNode, expansionStates);
    }
}

void ExplorerPanel::SetRootInstance(std::shared_ptr<Instance> root) {
    rootInstance = root;
    RefreshTree();
}

void ExplorerPanel::RenderTreeNode(std::shared_ptr<TreeNode> node, Rectangle& bounds, float& yOffset) {
    if (!node || !node->instance) return;
    
    // Skip nodes that don't match search filter
    if (!ShouldShowNode(node)) {
        // Still need to process children for search matches
        if (node->expanded) {
            for (auto& child : node->children) {
                RenderTreeNode(child, bounds, yOffset);
            }
        }
        return;
    }
    
    float nodeY = yOffset;
    float nodeHeight = NODE_HEIGHT;
    
    // Skip if node is outside visible area
    if (nodeY + nodeHeight < bounds.y || nodeY > bounds.y + bounds.height) {
        yOffset += nodeHeight;
        contentHeight += nodeHeight;
        
        // Still need to process children for content height calculation
        if (node->expanded) {
            for (auto& child : node->children) {
                RenderTreeNode(child, bounds, yOffset);
            }
        }
        return;
    }
    
    Rectangle nodeRect = {
        bounds.x + node->depth * INDENT_SIZE,
        nodeY,
        bounds.width - node->depth * INDENT_SIZE,
        nodeHeight
    };
    
    // Check if this node is selected
    bool isSelected = (selectedInstance == node->instance);
    bool isHovered = CheckCollisionPointRec(GetMousePosition(), nodeRect);
    
    // Draw node background
    Color bgColor = isSelected ? NODE_SELECTED_COLOR : 
                   isHovered ? NODE_HOVER_COLOR : NODE_NORMAL_COLOR;
    DrawRectangleRec(nodeRect, bgColor);
    
    // Handle click
    if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        HandleNodeClick(node, nodeRect);
    }
    
    // Handle right-click for context menu
    if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        Vector2 mousePos = GetMousePosition();
        ShowContextMenu(mousePos, node->instance);
    }
    
    float textX = nodeRect.x + TEXT_PADDING;
    
    // Draw expand/collapse arrow if node has children
    if (!node->children.empty()) {
        const char* arrow = node->expanded ? "v" : ">";
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), arrow, {textX, nodeY + 2}, 16, 1.2f, EXPAND_ARROW_COLOR);
        } else {
            DrawText(arrow, (int)textX, (int)(nodeY + 2), 16, EXPAND_ARROW_COLOR);
        }
        textX += ICON_SIZE;
    } else {
        textX += ICON_SIZE; // Keep alignment
    }
    // the first time that i did this, my brain cells is start to decreasingly wonderful!
    
    // Draw icon
    Texture2D iconTexture = GuiManager::GetInstanceIcon(node->instance->GetClassName());
    if (IsTextureValid(iconTexture)) {
        // Draw the actual icon texture
        Rectangle iconRect = {textX, nodeY + 2, 16, 16};
        DrawTexturePro(iconTexture, 
                      {0, 0, (float)iconTexture.width, (float)iconTexture.height}, 
                      iconRect, 
                      {0, 0}, 0.0f, WHITE);
    } else {
        // Fallback to text icon if texture not available
        const char* icon = GetNodeIcon(node->instance);
        Color iconColor = GetNodeColor(node->instance);
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), icon, {textX, nodeY + 2}, 16, 1.2f, iconColor);
        } else {
            DrawText(icon, (int)textX, (int)(nodeY + 2), 16, iconColor);
        }
    }
    textX += ICON_SIZE + TEXT_PADDING;
    
    // Draw node name (or rename text box)
    if (renameActive && renameTarget == node->instance) {
        // Draw rename background
        Rectangle renameRect = {textX - 2, nodeY + 1, bounds.width - (textX - bounds.x) - 4, nodeHeight - 2};
        DrawRectangleRec(renameRect, {50, 50, 50, 255}); // Darker background
        DrawRectangleLinesEx(renameRect, 1.0f, {70, 130, 180, 255}); // Blue border
        
        // Draw rename text
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), renameText.c_str(), {textX, nodeY + 4}, 18, 1.2f, TEXT_COLOR);
        } else {
            DrawText(renameText.c_str(), (int)textX, (int)(nodeY + 4), 18, TEXT_COLOR);
        }
        
        // Draw cursor with blinking animation
        if (fmod(renameBlinkTimer, 1.0) < 0.5) {
            int textWidth;
            if (GuiManager::IsCustomFontLoaded()) {
                Vector2 textSize = MeasureTextEx(GuiManager::GetCustomFont(), renameText.substr(0, renameCursorPos).c_str(), 18, 1.2f);
                textWidth = (int)textSize.x;
            } else {
                textWidth = MeasureText(renameText.substr(0, renameCursorPos).c_str(), 18);
            }
            DrawLine((int)(textX + textWidth), (int)(nodeY + 2), 
                    (int)(textX + textWidth), (int)(nodeY + nodeHeight - 2), TEXT_COLOR);
        }
    } else {
        // Draw normal node name
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), node->instance->Name.c_str(), {textX, nodeY + 4}, 18, 1.2f, TEXT_COLOR);
        } else {
            DrawText(node->instance->Name.c_str(), (int)textX, (int)(nodeY + 4), 18, TEXT_COLOR);
        }
    }
    
    yOffset += nodeHeight;
    contentHeight += nodeHeight;
    
    // Render all the children if expanded
    if (node->expanded) {
        for (auto& child : node->children) {
            RenderTreeNode(child, bounds, yOffset);
        }
    }
}

void ExplorerPanel::BuildTreeFromInstance(std::shared_ptr<Instance> instance, std::shared_ptr<TreeNode> parentNode, int depth) {
    if (!instance) return;
    
    parentNode->children.clear();
    
    for (auto& child : instance->GetChildren()) {
        if (!child || !child->Alive) continue;
        
        // Hide CurrentCamera from explorer display
        if (child->Name == "CurrentCamera" && child->Class == InstanceClass::Camera) {
            continue;
        }
        
        auto childNode = std::make_shared<TreeNode>();
        childNode->instance = child;
        childNode->depth = depth + 1;
        childNode->expanded = false; // Start collapsed
        
        // Recursively build children
        BuildTreeFromInstance(child, childNode, depth + 1);
        
        parentNode->children.push_back(childNode);
    }
}

bool ExplorerPanel::HandleNodeClick(std::shared_ptr<TreeNode> node, Rectangle nodeRect) {
    if (!node || !node->instance) return false;
    
    Vector2 mousePos = GetMousePosition();
    float expandAreaX = nodeRect.x + TEXT_PADDING;
    
    // Check if click is on expand/collapse area (only if node has children)
    if (!node->children.empty() && mousePos.x >= expandAreaX && mousePos.x <= expandAreaX + ICON_SIZE) {
        // Toggle expansion state
        node->expanded = !node->expanded;
        return true; // Don't select the instance when clicking expand arrow
    }
    
    // Otherwise, select the instance (but don't toggle expansion)
    SelectInstance(node->instance);
    return true;
}

void ExplorerPanel::SelectInstance(std::shared_ptr<Instance> instance) {
    selectedInstance = instance;
    if (guiManager) {
        guiManager->SetSelectedInstance(instance);
    }
}

Color ExplorerPanel::GetNodeColor(std::shared_ptr<Instance> instance) {
    if (!instance) return TEXT_COLOR;
    
    switch (instance->Class) {
        case InstanceClass::Workspace:
            return {100, 150, 255, 255}; // Blue
        case InstanceClass::Part:
        case InstanceClass::MeshPart:
            return {150, 255, 150, 255}; // Green
        case InstanceClass::Model:
            return {255, 200, 100, 255}; // Orange
        case InstanceClass::Script:
        case InstanceClass::LocalScript:
            return {255, 150, 150, 255}; // Red
        case InstanceClass::Folder:
            return {200, 200, 100, 255}; // Yellow
        case InstanceClass::Sky:
            return {150, 200, 255, 255}; // Light Blue
        default:
            return TEXT_COLOR;
    }
}

const char* ExplorerPanel::GetNodeIcon(std::shared_ptr<Instance> instance) {
    if (!instance) return "?";
    
    switch (instance->Class) {
        case InstanceClass::Workspace:
            return "W";
        case InstanceClass::Part:
            return "P";
        case InstanceClass::MeshPart:
            return "M";
        case InstanceClass::Model:
            return "O";
        case InstanceClass::Script:
            return "S";
        case InstanceClass::LocalScript:
            return "L";
        case InstanceClass::Folder:
            return "F";
        case InstanceClass::Camera:
            return "C";
        case InstanceClass::Sky:
            return "K";
        default:
            return "I";
    } // incase they couldn't find the icon...
}

void ExplorerPanel::StoreExpansionStates(std::shared_ptr<TreeNode> node, std::unordered_map<std::string, bool>& states) {
    if (!node || !node->instance) return;
    
    // Create a unique key for this instance (using name + class as identifier)
    std::string key = node->instance->Name + "_" + std::to_string(static_cast<int>(node->instance->Class));
    states[key] = node->expanded;
    
    // Recursively store children states
    for (auto& child : node->children) {
        StoreExpansionStates(child, states);
    }
}

void ExplorerPanel::RestoreExpansionStates(std::shared_ptr<TreeNode> node, const std::unordered_map<std::string, bool>& states) {
    if (!node || !node->instance) return;
    
    // Create the same unique key
    std::string key = node->instance->Name + "_" + std::to_string(static_cast<int>(node->instance->Class));
    auto it = states.find(key);
    if (it != states.end()) {
        node->expanded = it->second;
    }
    
    // Recursively restore children states
    for (auto& child : node->children) {
        RestoreExpansionStates(child, states);
    }
}

bool ExplorerPanel::MatchesSearch(std::shared_ptr<Instance> instance, const std::string& query) {
    if (!instance || query.empty()) return true;
    
    // Convert both to lowercase for case-insensitive search
    std::string instanceName = instance->Name;
    std::string searchQuery = query;
    
    std::transform(instanceName.begin(), instanceName.end(), instanceName.begin(), ::tolower);
    std::transform(searchQuery.begin(), searchQuery.end(), searchQuery.begin(), ::tolower);
    
    // Check if instance name contains the search query
    return instanceName.find(searchQuery) != std::string::npos;
}

bool ExplorerPanel::ShouldShowNode(std::shared_ptr<TreeNode> node) {
    if (!node || !node->instance) return false;
    
    // If no search query, show all nodes
    if (searchQuery.empty()) return true;
    
    // If this node matches, show it
    if (MatchesSearch(node->instance, searchQuery)) return true;
    
    // If any child matches, show this node (so we can see the path to matching children)
    for (auto& child : node->children) {
        if (ShouldShowNode(child)) return true;
    }
    
    return false;
}

void ExplorerPanel::BuildVisibleNodesList() {
    visibleNodes.clear();
    if (rootNode) {
        CollectVisibleNodes(rootNode, visibleNodes);
    }
}

void ExplorerPanel::CollectVisibleNodes(std::shared_ptr<TreeNode> node, std::vector<std::shared_ptr<TreeNode>>& nodes) {
    if (!node || !ShouldShowNode(node)) return;
    
    nodes.push_back(node);
    
    if (node->expanded) {
        for (auto& child : node->children) {
            CollectVisibleNodes(child, nodes);
        }
    }
}

void ExplorerPanel::NavigateUp() {
    if (visibleNodes.empty()) return;
    
    if (selectedNodeIndex > 0) {
        selectedNodeIndex--;
        auto selectedNode = visibleNodes[selectedNodeIndex];
        SelectInstance(selectedNode->instance);
    }
}

void ExplorerPanel::NavigateDown() {
    if (visibleNodes.empty()) return;
    
    if (selectedNodeIndex < (int)visibleNodes.size() - 1) {
        selectedNodeIndex++;
        auto selectedNode = visibleNodes[selectedNodeIndex];
        SelectInstance(selectedNode->instance);
    }
}

void ExplorerPanel::UpdateSelectedNodeIndex() {
    if (!selectedInstance || visibleNodes.empty()) {
        selectedNodeIndex = -1;
        return;
    }
    
    for (int i = 0; i < (int)visibleNodes.size(); i++) {
        if (visibleNodes[i]->instance == selectedInstance) {
            selectedNodeIndex = i;
            return;
        }
    }
    
    selectedNodeIndex = -1;
}

// Context menu implementation
void ExplorerPanel::RenderContextMenu(Rectangle bounds) {
    if (!contextMenuActive || !contextMenuTarget) return;
    
    // Context menu items
    std::vector<std::string> menuItems = {"Rename", "Insert Instance..."};
    float menuHeight = menuItems.size() * CONTEXT_MENU_ITEM_HEIGHT + 4; // +4 for padding
    
    // Adjust position if menu goes off screen
    Vector2 menuPos = contextMenuPosition;
    if (menuPos.x + CONTEXT_MENU_WIDTH > bounds.x + bounds.width) {
        menuPos.x = bounds.x + bounds.width - CONTEXT_MENU_WIDTH;
    }
    if (menuPos.y + menuHeight > bounds.y + bounds.height) {
        menuPos.y = bounds.y + bounds.height - menuHeight;
    }
    
    Rectangle menuRect = {menuPos.x, menuPos.y, CONTEXT_MENU_WIDTH, menuHeight};
    
    // Draw menu background and border
    DrawRectangleRec(menuRect, {40, 40, 40, 255});
    DrawRectangleLinesEx(menuRect, 1.0f, {70, 70, 70, 255});
    
    // Draw menu items
    for (size_t i = 0; i < menuItems.size(); i++) {
        Rectangle itemRect = {
            menuPos.x + 2,
            menuPos.y + 2 + i * CONTEXT_MENU_ITEM_HEIGHT,
            CONTEXT_MENU_WIDTH - 4,
            CONTEXT_MENU_ITEM_HEIGHT
        };
        
        bool isHovered = CheckCollisionPointRec(GetMousePosition(), itemRect);
        
        // Draw item background
        if (isHovered) {
            DrawRectangleRec(itemRect, {60, 60, 60, 255});
        }
        
        // Draw item text
        const char* itemText = menuItems[i].c_str();
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), itemText, 
                      {itemRect.x + 8, itemRect.y + 4}, 16, 1.2f, TEXT_COLOR);
        } else {
            DrawText(itemText, (int)(itemRect.x + 8), (int)(itemRect.y + 4), 16, TEXT_COLOR);
        }
        
        // Draw arrow for submenu
        if (menuItems[i] == "Insert Instance...") {
            const char* arrow = ">";
            if (GuiManager::IsCustomFontLoaded()) {
                DrawTextEx(GuiManager::GetCustomFont(), arrow, 
                          {itemRect.x + itemRect.width - 16, itemRect.y + 4}, 16, 1.2f, TEXT_COLOR);
            } else {
                DrawText(arrow, (int)(itemRect.x + itemRect.width - 16), (int)(itemRect.y + 4), 16, TEXT_COLOR);
            }
            
            // Show submenu if hovered
            if (isHovered && !insertSubmenuActive) {
                insertSubmenuActive = true;
                insertSubmenuPosition = {itemRect.x + itemRect.width, itemRect.y};
            }
        } else if (insertSubmenuActive && isHovered) {
            // Hide submenu if hovering over other items
            insertSubmenuActive = false;
        }
    }
}

void ExplorerPanel::RenderInsertSubmenu(Rectangle bounds) {
    if (!insertSubmenuActive) return;
    
    auto availableTypes = GetAvailableInstanceTypes();
    float submenuHeight = availableTypes.size() * CONTEXT_MENU_ITEM_HEIGHT + 4;
    
    // Adjust position if submenu goes off screen
    Vector2 submenuPos = insertSubmenuPosition;
    if (submenuPos.x + SUBMENU_WIDTH > bounds.x + bounds.width) {
        submenuPos.x = contextMenuPosition.x - SUBMENU_WIDTH;
    }
    if (submenuPos.y + submenuHeight > bounds.y + bounds.height) {
        submenuPos.y = bounds.y + bounds.height - submenuHeight;
    }
    
    Rectangle submenuRect = {submenuPos.x, submenuPos.y, SUBMENU_WIDTH, submenuHeight};
    
    // Draw submenu background and border
    DrawRectangleRec(submenuRect, {35, 35, 35, 255});
    DrawRectangleLinesEx(submenuRect, 1.0f, {70, 70, 70, 255});
    
    // Draw submenu items
    for (size_t i = 0; i < availableTypes.size(); i++) {
        Rectangle itemRect = {
            submenuPos.x + 2,
            submenuPos.y + 2 + i * CONTEXT_MENU_ITEM_HEIGHT,
            SUBMENU_WIDTH - 4,
            CONTEXT_MENU_ITEM_HEIGHT
        };
        
        bool isHovered = CheckCollisionPointRec(GetMousePosition(), itemRect);
        
        // Draw item background
        if (isHovered) {
            DrawRectangleRec(itemRect, {55, 55, 55, 255});
        }
        
        // Draw item text
        const char* itemText = availableTypes[i].c_str();
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), itemText, 
                      {itemRect.x + 8, itemRect.y + 4}, 14, 1.2f, TEXT_COLOR);
        } else {
            DrawText(itemText, (int)(itemRect.x + 8), (int)(itemRect.y + 4), 14, TEXT_COLOR);
        }
    }
}

bool ExplorerPanel::HandleContextMenuClick(Vector2 mousePos) {
    if (contextMenuActive) {
        // Check main context menu
        std::vector<std::string> menuItems = {"Rename", "Insert Instance..."};
        float menuHeight = menuItems.size() * CONTEXT_MENU_ITEM_HEIGHT + 4;
        
        Vector2 menuPos = contextMenuPosition;
        Rectangle menuRect = {menuPos.x, menuPos.y, CONTEXT_MENU_WIDTH, menuHeight};
        
        if (CheckCollisionPointRec(mousePos, menuRect)) {
            // Click is inside context menu
            for (size_t i = 0; i < menuItems.size(); i++) {
                Rectangle itemRect = {
                    menuPos.x + 2,
                    menuPos.y + 2 + i * CONTEXT_MENU_ITEM_HEIGHT,
                    CONTEXT_MENU_WIDTH - 4,
                    CONTEXT_MENU_ITEM_HEIGHT
                };
                
                if (CheckCollisionPointRec(mousePos, itemRect)) {
                    if (menuItems[i] == "Rename") {
                        StartRename(contextMenuTarget);
                        HideContextMenu();
                        return true;
                    }
                    // "Insert Instance..." doesn't need action here - submenu handles it
                    return true;
                }
            }
            return true;
        }
    }
    
    if (insertSubmenuActive) {
        // Check insert submenu
        auto availableTypes = GetAvailableInstanceTypes();
        float submenuHeight = availableTypes.size() * CONTEXT_MENU_ITEM_HEIGHT + 4;
        
        Vector2 submenuPos = insertSubmenuPosition;
        Rectangle submenuRect = {submenuPos.x, submenuPos.y, SUBMENU_WIDTH, submenuHeight};
        
        if (CheckCollisionPointRec(mousePos, submenuRect)) {
            // Click is inside submenu
            for (size_t i = 0; i < availableTypes.size(); i++) {
                Rectangle itemRect = {
                    submenuPos.x + 2,
                    submenuPos.y + 2 + i * CONTEXT_MENU_ITEM_HEIGHT,
                    SUBMENU_WIDTH - 4,
                    CONTEXT_MENU_ITEM_HEIGHT
                };
                
                if (CheckCollisionPointRec(mousePos, itemRect)) {
                    // Create new instance
                    std::string typeName = availableTypes[i];
                    auto newInstance = Instance::New(typeName);
                    if (newInstance && contextMenuTarget) {
                        newInstance->SetParent(contextMenuTarget);
                        LOGI("Created new %s instance in %s", typeName.c_str(), contextMenuTarget->Name.c_str());
                    }
                    HideContextMenu();
                    return true;
                }
            }
            return true;
        }
    }
    
    return false; // Click outside menu
}

void ExplorerPanel::ShowContextMenu(Vector2 position, std::shared_ptr<Instance> target) {
    contextMenuActive = true;
    contextMenuPosition = position;
    contextMenuTarget = target;
    insertSubmenuActive = false;
}

void ExplorerPanel::HideContextMenu() {
    contextMenuActive = false;
    insertSubmenuActive = false;
    contextMenuTarget = nullptr;
}

void ExplorerPanel::StartRename(std::shared_ptr<Instance> target) {
    if (!target) return;
    
    renameActive = true;
    renameTarget = target;
    renameText = target->Name;
    renameCursorPos = (int)renameText.length();
    renameBlinkTimer = 0.0;
}

void ExplorerPanel::FinishRename() {
    if (renameActive && renameTarget && !renameText.empty()) {
        renameTarget->SetName(renameText);
        LOGI("Renamed instance to '%s'", renameText.c_str());
    }
    
    renameActive = false;
    renameTarget = nullptr;
    renameText.clear();
    renameCursorPos = 0;
}

void ExplorerPanel::CancelRename() {
    renameActive = false;
    renameTarget = nullptr;
    renameText.clear();
    renameCursorPos = 0;
}

std::vector<std::string> ExplorerPanel::GetAvailableInstanceTypes() {
    std::vector<std::string> types;
    
    // Get all registered instance types from the Instance registry
    auto allTypes = Instance::GetRegisteredTypes();
    for (const std::string& typeName : allTypes) {
        // Filter out service types that shouldn't be created manually
        if (typeName != "Game" && typeName != "Workspace" && 
            typeName != "RunService" && typeName != "Lighting" &&
            typeName != "UserInputService" && typeName != "TweenService") {
            types.push_back(typeName);
        }
    }
    
    // Sort alphabetically for better UX
    std::sort(types.begin(), types.end());
    
    return types;
}
