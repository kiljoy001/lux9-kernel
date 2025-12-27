public class Point
{
    public int X;
    public int Y;
    
    public Point()
    {
        X = 0;
        Y = 0;
    }
    
    public Point(int x, int y)
    {
        X = x;
        Y = y;
    }
    
    public int Sum()
    {
        return X + Y;
    }
}

public class Program
{
    public static int Main()
    {
        Point p = new Point();
        p.X = 10;
        p.Y = 20;
        int result = p.Sum();
        return result; // Should return 30
    }
}
