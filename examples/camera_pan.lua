--&clientscript
local UserInputService = game:GetService("UserInputService")
local RunService = game:GetService("RunService")
local Camera = workspace.CurrentCamera

local holding = false
local sensitivity = 0.002 -- lower = slower rotation
local rotation = Vector2.new(0, 0) -- X = yaw, Y = pitch

-- Save default MouseBehavior
local defaultBehavior = UserInputService.MouseBehavior

local function createLargeCheckerBoard(tileCount, tileSize, parent)
    local board = workspace
    local plate = {}
    for x = 0, tileCount - 1 do
        for z = 0, tileCount - 1 do
            local part = Instance.new("Part")
            part.Size = tileSize
            part.Anchored = true
            part.Position = Vector3.new(
                x * tileSize.X,
                -10,
                z * tileSize.Z
            )
            
            -- Alternate colors per tile
            if (x + z) % 2 == 0 then
                part.Color = Color3.fromRGB(255, 255, 255) -- white
            else
                part.Color = Color3.fromRGB(0, 0, 0) -- black
            end

            plate[part.Name] = part
            part.Parent = board
        end
    end
    return plate
end

local PlateModel = Instance.new("Model")
PlateModel.Name = "CheckerBoard"
PlateModel.Parent = workspace
for i,v in pairs( createLargeCheckerBoard(16, Vector3.new(4,1,4), PlateModel) ) do
    v.Parent = PlateModel
end
PlateModel:PivotTo(Camera.CFrame)

-- When RMB pressed
UserInputService.InputBegan:Connect(function(input, gp)
	if gp then return end
	if input.UserInputType == Enum.UserInputType.MouseButton2 then
		holding = true
		UserInputService.MouseBehavior = Enum.MouseBehavior.LockCenter
	end
end)

-- When RMB released
UserInputService.InputEnded:Connect(function(input, gp)
	if input.UserInputType == Enum.UserInputType.MouseButton2 then
		holding = false
		UserInputService.MouseBehavior = defaultBehavior
	end
end)

local Baseplate = workspace:FindFirstChild("Baseplate")
if Baseplate then
    print("Hiding existing Baseplate")
    Baseplate.Position = Vector3.new(0, -10000, 0)
end

-- Main update loop
RunService.RenderStepped:Connect(function()
	if holding then
		local delta = UserInputService:GetMouseDelta()
		
		-- Update yaw (X) and pitch (Y)
		rotation = rotation + Vector2.new(-delta.X * sensitivity, -delta.Y * sensitivity)
		
		-- Clamp pitch to avoid flipping (looking straight up/down)
		rotation = Vector2.new(rotation.X, math.clamp(rotation.Y, -math.rad(80), math.rad(80)))
		
		-- Apply camera rotation (keep position the same)
		local cameraCFrame = CFrame.new(Camera.CFrame.Position)
			* CFrame.Angles(0, -rotation.X, 0) -- yaw
			* CFrame.Angles(rotation.Y, 0, 0) -- pitch
		
		Camera.CFrame = cameraCFrame
	end
end)
