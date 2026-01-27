// Minimal Hello World for Lux9 WASM testing
// Just return 42 - no method calls, no host imports

public static class Hello
{
    public static int KernelEntry()
    {
        return 42;
    }
}
