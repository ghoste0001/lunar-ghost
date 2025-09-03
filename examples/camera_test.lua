--&clientscript
local RunService = game:GetService("RunService")
local UserInputService = game:GetService("UserInputService")
local Camera = workspace.CurrentCamera

local targetPosition = Vector3.new(0, 6, 0)

local Model = Instance.new("Model")
Model.Parent = workspace
Model.Name = "Baseplate"

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

RunService.RenderStepped:Connect(function(dt)
	local CameraValue = math.rad(math.sin(os.clock() * 2) * 0.01)
	Camera.CFrame = Camera.CFrame * CFrame.Angles(
		0,
		math.rad(math.cos(os.clock() * 2) * 0.01),
		CameraValue
	)
end)
