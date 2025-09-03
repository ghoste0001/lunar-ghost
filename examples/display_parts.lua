	--&clientscript
local RunService = game:GetService("RunService")
local UserInputService = game:GetService("UserInputService")
local Camera = workspace.CurrentCamera

local targetPosition = Vector3.new(0, 6, 0)

local Model = Instance.new("Model")
local Model2 = Instance.new("Model")
Model.Parent = workspace
Model.Name = "Baseplate"
Model2.Parent = workspace
Model2.Name = "Rotating"

local function createCheckerBoard(tileCount, tileSize, parent)
	for x = 0, tileCount - 1 do
		for z = 0, tileCount - 1 do
			local part = Instance.new("Part")
			part.Size = tileSize
			part.Anchored = true
			part.Position = Vector3.new(
				x * tileSize.X,
				0,
				z * tileSize.Z
			)
			if (x + z) % 2 == 0 then
				part.Color = Color3.fromRGB(255, 255, 255)
			else
				part.Color = Color3.fromRGB(150, 150, 150)
			end
			part.Parent = parent
		end
	end
end

Camera.CFrame = CFrame.new(targetPosition)

createCheckerBoard(64, Vector3.new(4, 1, 4), Model)
Model:PivotTo(CFrame.new(0,0,0))

local Part = Instance.new("Part")
local Wedge = Instance.new("Part")
local Ball = Instance.new("Part")
local Cylinder = Instance.new("Part")

Part.Size = Vector3.new(2, 2, 2)
Part.Position = Vector3.new(6, 4, 8)
Part.Parent = Model2

Ball.Size = Vector3.new(2,2,2)
Ball.Position = Vector3.new(-6, 3, 8)
Ball.Parent = Model2
Ball.Shape = Enum.PartType.Ball

Wedge.Size = Vector3.new(2, 2, 2)
Wedge.Position = Vector3.new(0, 3, 8)
Wedge.Parent = Model2
Wedge.Shape = Enum.PartType.Wedge

Cylinder.Size = Vector3.new(2,2,2)
Cylinder.Position = Vector3.new(-6, -3, 8)
Cylinder.Parent = Model2
Cylinder.Shape = Enum.PartType.Cylinder

-- ===== Camera Rotation State =====
local holding = false
local sensitivity = 0.002
local rotation = Vector2.new(0, 0) -- yaw, pitch
local ModelTarget = CFrame.new()
local defaultBehavior = UserInputService.MouseBehavior

UserInputService.InputBegan:Connect(function(input, gp)
	if gp then return end
	if input.UserInputType == Enum.UserInputType.MouseButton2 then
		holding = true
		UserInputService.MouseBehavior = Enum.MouseBehavior.LockCenter
	end
end)

UserInputService.InputEnded:Connect(function(input, gp)
	if input.UserInputType == Enum.UserInputType.MouseButton2 then
		holding = false
		UserInputService.MouseBehavior = defaultBehavior
	end
end)

-- ===== Main Update Loop =====
RunService.RenderStepped:Connect(function(dt)
    -- Rotate Model2 for visual effect
    for i, v in pairs(Model2:GetChildren()) do
        v.CFrame = v.CFrame * CFrame.Angles(math.rad(0.01), math.rad(0.01), 0)
    end

    -- ===== Camera Rotation =====
    if holding then
        local delta = UserInputService:GetMouseDelta()
        rotation = rotation + Vector2.new(-delta.X * sensitivity, -delta.Y * sensitivity)
        rotation = Vector2.new(rotation.X, math.clamp(rotation.Y, -math.rad(80), math.rad(80)))
    end

    -- Apply rotation only, no movement
    Camera.CFrame = CFrame.new(targetPosition)
                     * CFrame.Angles(0, -rotation.X, 0) -- yaw
                     * CFrame.Angles(rotation.Y, 0, 0)  -- pitch

    -- Keep Model2 slightly in front of camera
	ModelTarget = ModelTarget:Lerp(
		Camera.CFrame * CFrame.new(0, 0, 8),
		0.05
	)
	Model2:PivotTo(ModelTarget)
end)
