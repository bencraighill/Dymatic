namespace Dymatic
{
    public enum GamepadButtonCode
    {
        // From glfw3.h
        Invalid = -1,

        Cross = 0,
        Circle = 1,
        Square = 2,
        Triangle = 3,
        LeftBumper = 4,
        RightBumper = 5,
        LeftTrigger = 6,
        RightTrigger = 7,
        Share = 8,
        Options = 9,
        LeftThumb = 10,
        RightThumb = 11,
        Playstation = 12,
        TouchPad = 13,
        Mic = 14,
        DPadUp = 15,
        DPadRight = 16,
        DPadDown = 17,
        DPadLeft = 18
    }

    public enum GamepadAxisCode
    {
        // From glfw3.h
        Invalid = -1,

        LeftXAxis = 0,
        LeftYAxis = 1,
        RightXAxis = 2,
        RightYAxis = 3,
        LeftTriggerAxis = 4,
        RightTriggerAxis = 5
    }

    public enum GamepadSensorCode
    {
        // From SDL_gamecontroller.h
        Invalid = -1,
        Unknown = 0,

        Accelerometer = 1,
        Gyroscope = 2,

        // For Joy-Con controllers
        AccelerometerLeft = 3,
        GyroscopeLeft = 4,
        AccelerometerRight = 5,
        GyroscopeRight = 6
    }
}