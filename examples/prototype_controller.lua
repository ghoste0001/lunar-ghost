--&clientscript
-- Prototype of a free-fly camera controller
-- - Right-click and drag to look around
-- - WASD to move, Space to jump

local UserInputService = game:GetService("UserInputService")
local RunService = game:GetService("RunService")
local Camera = workspace.CurrentCamera

local holding = false
local sensitivity = 0.002
local rotation = Vector2.new(0, 0)

local WalkSpeed = 16
local moveSpeed = 0.05 * (WalkSpeed/16)
local smoothness = 0.1
local moveDirection = Vector3.zero
local targetPosition = Vector3.new(0, 10, -20)
local currentPosition = targetPosition

local gravity = -50
local jumpPower = 20
local verticalVelocity = 0
local isGrounded = false
local groundY = -5 + 0.5

local defaultBehavior = UserInputService.MouseBehavior

local function createCheckerBoard(tileCount, tileSize, parent)
	for x = 0, tileCount - 1 do
		for z = 0, tileCount - 1 do
			local part = Instance.new("Part")
			part.Size = tileSize
			part.Anchored = true
			part.Position = Vector3.new(
				x * tileSize.X,
				-5,
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

createCheckerBoard(64, Vector3.new(4, 1, 4), workspace)

local keysDown = {
	W = false, A = false, S = false, D = false
}

UserInputService.InputBegan:Connect(function(input, gp)
	if gp then return end
	if input.UserInputType == Enum.UserInputType.MouseButton2 then
		holding = true
		UserInputService.MouseBehavior = Enum.MouseBehavior.LockCenter
	elseif input.KeyCode == Enum.KeyCode.W then
		keysDown.W = true
	elseif input.KeyCode == Enum.KeyCode.A then
		keysDown.A = true
	elseif input.KeyCode == Enum.KeyCode.S then
		keysDown.S = true
	elseif input.KeyCode == Enum.KeyCode.D then
		keysDown.D = true
	elseif input.KeyCode == Enum.KeyCode.Space then
		if isGrounded then
			verticalVelocity = jumpPower
			isGrounded = false
		end
	end
end)

UserInputService.InputEnded:Connect(function(input, gp)
	if input.UserInputType == Enum.UserInputType.MouseButton2 then
		holding = false
		UserInputService.MouseBehavior = defaultBehavior
	elseif input.KeyCode == Enum.KeyCode.W then
		keysDown.W = false
	elseif input.KeyCode == Enum.KeyCode.A then
		keysDown.A = false
	elseif input.KeyCode == Enum.KeyCode.S then
		keysDown.S = false
	elseif input.KeyCode == Enum.KeyCode.D then
		keysDown.D = false
	end
end)

RunService.RenderStepped:Connect(function(dt)
	if holding then
		local delta = UserInputService:GetMouseDelta()
		rotation = rotation + Vector2.new(-delta.X * sensitivity, -delta.Y * sensitivity)
		rotation = Vector2.new(rotation.X, math.clamp(rotation.Y, -math.rad(80), math.rad(80)))
	end

	moveDirection = Vector3.zero
	local camYaw = CFrame.Angles(0, -rotation.X, 0)

	if keysDown.W then moveDirection += Vector3.new(0, 0, 1) end
	if keysDown.S then moveDirection += Vector3.new(0, 0, -1) end
	if keysDown.A then moveDirection += Vector3.new(1, 0, 0) end
	if keysDown.D then moveDirection += Vector3.new(-1, 0, 0) end

	if moveDirection.Magnitude > 0 then
		moveDirection = (camYaw:VectorToWorldSpace(moveDirection.Unit)) * moveSpeed
	end

	targetPosition += moveDirection

    verticalVelocity = verticalVelocity + gravity * dt
    targetPosition = Vector3.new(
        targetPosition.X,
        targetPosition.Y + verticalVelocity * dt,
        targetPosition.Z
    )

    if targetPosition.Y <= groundY + 5 then
        targetPosition = Vector3.new(targetPosition.X, groundY + 5, targetPosition.Z)
        verticalVelocity = 0
        isGrounded = true
    end

	currentPosition = currentPosition:Lerp(targetPosition, smoothness)

	local cameraCFrame =
		CFrame.new(currentPosition)
		* CFrame.Angles(0, -rotation.X, 0)
		* CFrame.Angles(rotation.Y, 0, 0)

	Camera.CFrame = cameraCFrame
end)
