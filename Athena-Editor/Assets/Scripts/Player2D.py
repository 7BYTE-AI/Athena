from Athena_Core import *


class Player2D(Entity):
    _translation = Vector3()
    _rigidbody2D = Rigidbody2DComponent()
    _camera = Entity()
    speed = 0.1

    def OnCreate(self):
        Log.Warn(f"Player2D::OnCreate() - {self._ID.AsUInt64()}")
        self._translation = self.GetComponent(TransformComponent).translation
        self._rigidbody2D = self.GetComponent(Rigidbody2DComponent)
        self._camera = Entity.FindEntityByName("Camera")     # type of 'Camera'

    def OnUpdate(self, frameTime):
        velocity = Vector2()

        if Input.IsKeyPressed(Keyboard.D):
            velocity.x += 1
        elif Input.IsKeyPressed(Keyboard.A):
            velocity.x -= 1
        
        if Input.IsKeyPressed(Keyboard.Space):
            velocity.y += 2
        
        velocity *= self.speed * frameTime.AsSeconds()
        self._rigidbody2D.ApplyLinearImpulseToCenter(velocity, True)

        self._camera.SetPosition(self._translation)
