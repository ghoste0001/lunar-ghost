--&clientscript
local RunService = game:GetService("RunService")
local UserInputService = game:GetService("UserInputService")
local Camera = workspace.CurrentCamera

local targetPosition = Vector3.new(0, 6, 0)

Camera.CFrame = CFrame.new(targetPosition)

local Part = Instance.new("Part")
Part.Parent = workspace
Part.CFrame =  Camera.CFrame * CFrame.new(0, 0, 15)
Part.Size = Vector3.new(4,4,4)

local Model = Instance.new("Model")
Model.Parent = workspace
Model.Name = "Baseplate"

local DefaultFOV = Camera.FieldOfView
local DefaultPosition = Part.Position
local Distance = (DefaultPosition - Camera.CFrame.Position).Magnitude
local Alpha = 1

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

createCheckerBoard(64, Vector3.new(4, 1, 4), Model)
Model:PivotTo(CFrame.new(0,0,0))

print(Alpha)

RunService.RenderStepped:Connect(function(dt)
	Part.Position = DefaultPosition + Vector3.new(
		math.cos(os.clock() * 5),
		math.sin(os.clock() * 5),
		math.abs(math.cos(os.clock() * 0.5) * 50)
	)

	local currentDistance = (Part.Position - Camera.CFrame.Position).Magnitude
	Alpha = Distance / currentDistance
	
	Camera.CFrame = CFrame.new(Camera.CFrame.Position, Part.Position)
	Camera.FieldOfView = DefaultFOV * Alpha
end)