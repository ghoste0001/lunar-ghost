#pragma once
#include <raylib.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include "bootstrap/Instance.h"

class GuiManager;

class ExplorerPanel {
public:
    ExplorerPanel(GuiManager* manager);
    ~ExplorerPanel();

    void Render(Rectangle bounds);
    void Update();

    // Tree node management
    void RefreshTree();
    void SetRootInstance(std::shared_ptr<Instance> root);

private:
    GuiManager* guiManager;
    std::shared_ptr<Instance> rootInstance;
    
    // Tree state
    struct TreeNode {
        std::shared_ptr<Instance> instance;
        bool expanded = false;
        int depth = 0;
        std::vector<std::shared_ptr<TreeNode>> children;
    };
    
    std::shared_ptr<TreeNode> rootNode;
    std::shared_ptr<Instance> selectedInstance;
    
    // Rendering
    void RenderTreeNode(std::shared_ptr<TreeNode> node, Rectangle& bounds, float& yOffset);
    void BuildTreeFromInstance(std::shared_ptr<Instance> instance, std::shared_ptr<TreeNode> parentNode, int depth);
    
    // Interaction
    bool HandleNodeClick(std::shared_ptr<TreeNode> node, Rectangle nodeRect);
    void SelectInstance(std::shared_ptr<Instance> instance);
    
    // Expansion state management
    void StoreExpansionStates(std::shared_ptr<TreeNode> node, std::unordered_map<std::string, bool>& states);
    void RestoreExpansionStates(std::shared_ptr<TreeNode> node, const std::unordered_map<std::string, bool>& states);
    
    // Styling
    Color GetNodeColor(std::shared_ptr<Instance> instance);
    const char* GetNodeIcon(std::shared_ptr<Instance> instance);
    
    // Constants
    static constexpr float NODE_HEIGHT = 20.0f;
    static constexpr float INDENT_SIZE = 16.0f;
    static constexpr float ICON_SIZE = 16.0f;
    static constexpr float TEXT_PADDING = 4.0f;
    
    // Dark theme colors
    // FUCK OFF PLEASE STOP MESSIGN IT IS I  DO IT CORRECT NOW
    static constexpr Color BACKGROUND_COLOR = {30, 30, 30, 255};
    static constexpr Color NODE_NORMAL_COLOR = {45, 45, 45, 255};
    static constexpr Color NODE_HOVER_COLOR = {60, 60, 60, 255};
    static constexpr Color NODE_SELECTED_COLOR = {70, 130, 180, 255};
    static constexpr Color TEXT_COLOR = {220, 220, 220, 255};
    static constexpr Color EXPAND_ARROW_COLOR = {180, 180, 180, 255};
    
    // Search functionality
    std::string searchQuery;
    bool searchActive = false;
    double searchBlinkTimer = 0.0;
    int searchCursorPos = 0;
    
    // Navigation functionality
    std::vector<std::shared_ptr<TreeNode>> visibleNodes;
    int selectedNodeIndex = -1;
    
    // Helper methods for search
    bool MatchesSearch(std::shared_ptr<Instance> instance, const std::string& query);
    bool ShouldShowNode(std::shared_ptr<TreeNode> node);
    
    // Helper methods for navigation
    void BuildVisibleNodesList();
    void CollectVisibleNodes(std::shared_ptr<TreeNode> node, std::vector<std::shared_ptr<TreeNode>>& nodes);
    void NavigateUp();
    void NavigateDown();
    void UpdateSelectedNodeIndex();
    
    // Scroll state
    float scrollY = 0.0f;
    float contentHeight = 0.0f;
    bool isDragging = false; // i think keep this fucking "False" because i fucking hate it rework many time, wasted my fucking times
    // hell yeah draggable and why IT STICK WITH CURSOR BURH
    // ok from 100 lines now only 1 lines i hope it now working,
    Vector2 lastMousePos = {0, 0};
    
    // Context menu state
    bool contextMenuActive = false;
    Vector2 contextMenuPosition = {0, 0};
    std::shared_ptr<Instance> contextMenuTarget = nullptr;
    bool insertSubmenuActive = false;
    Vector2 insertSubmenuPosition = {0, 0};
    
    // Rename state
    bool renameActive = false;
    std::shared_ptr<Instance> renameTarget = nullptr;
    std::string renameText;
    int renameCursorPos = 0;
    double renameBlinkTimer = 0.0;
    
    // Context menu methods
    void RenderContextMenu(Rectangle bounds);
    void RenderInsertSubmenu(Rectangle bounds);
    bool HandleContextMenuClick(Vector2 mousePos);
    void ShowContextMenu(Vector2 position, std::shared_ptr<Instance> target);
    void HideContextMenu();
    void StartRename(std::shared_ptr<Instance> target);
    void FinishRename();
    void CancelRename();
    std::vector<std::string> GetAvailableInstanceTypes();
    
    // Context menu constants
    static constexpr float CONTEXT_MENU_WIDTH = 150.0f;
    static constexpr float CONTEXT_MENU_ITEM_HEIGHT = 24.0f;
    static constexpr float SUBMENU_WIDTH = 120.0f;
};
