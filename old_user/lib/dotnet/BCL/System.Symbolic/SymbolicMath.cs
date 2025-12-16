using System;

namespace System.Symbolic;

public static class SymbolicMath
{
    // Basic differentiation: d/dx (expression)
    public static ISymbolic Derive(ISymbolic exp, Symbol x)
    {
        if (exp is Fixnum) return new Fixnum(0);
        if (exp is Symbol s) return s.Name == x.Name ? new Fixnum(1) : new Fixnum(0);

        if (exp is Cell c)
        {
            var op = c.Car as Symbol;
            var args = c.Cdr as Cell;
            if (op == null || args == null) return Nil.Instance;

            var u = args.Car;
            var v = (args.Cdr as Cell)?.Car ?? new Fixnum(0);

            if (op.Name == "+")
            {
                // d(u+v) = du + dv
                return List(new Symbol("+"), Derive(u, x), Derive(v, x));
            }
            if (op.Name == "*")
            {
                // d(u*v) = u*dv + v*du
                return List(new Symbol("+"),
                    List(new Symbol("*"), u, Derive(v, x)),
                    List(new Symbol("*"), v, Derive(u, x))
                );
            }
        }
        return Nil.Instance;
    }

    // Helper to create list (cell chain)
    public static ISymbolic List(params ISymbolic[] elements)
    {
        ISymbolic head = Nil.Instance;
        for (int i = elements.Length - 1; i >= 0; i--)
        {
            head = new Cell(elements[i], head);
        }
        return head;
    }
}

public class Fixnum : Atom
{
    public int Value { get; }
    public Fixnum(int value) { Value = value; }
    public override string ToString() => Value.ToString();
}
