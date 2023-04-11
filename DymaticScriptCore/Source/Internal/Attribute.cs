using System;

namespace Dymatic
{
    [AttributeUsage(AttributeTargets.Parameter)]
    public class ParameterName : System.Attribute
    {
        public string Name { get; set; }
        public ParameterName(string name) { Name = name; }
    }

    [AttributeUsage(AttributeTargets.Parameter)]
    public class NoPinLabel : System.Attribute { }

    [AttributeUsage(AttributeTargets.Method)]
    public class DisplayName : System.Attribute
    {
        public string Name { get; set; }
        public DisplayName(string name) { Name = name; }
    }

    [AttributeUsage(AttributeTargets.Method)]
    public class IsPure : System.Attribute { }

    [AttributeUsage(AttributeTargets.Method)]
    public class IsEditorCallable : System.Attribute { }

    [AttributeUsage(AttributeTargets.Method)]
    public class IsConversion : System.Attribute { }

    [AttributeUsage(AttributeTargets.Method)]
    public class IsCompactNode : System.Attribute { }

    [AttributeUsage(AttributeTargets.Method)]
    public class NoPinLabels : System.Attribute { }

    [AttributeUsage(AttributeTargets.Method)]
    public class MethodCategory : System.Attribute
    {
        public string Category { get; set; }
        public MethodCategory(string category) { Category = category; }
    }

    [AttributeUsage(AttributeTargets.Method)]
    public class MethodKeywords : System.Attribute
    {
        public string Keywords { get; set; }
        public MethodKeywords(string keywords) { Keywords = keywords; }
    }
}