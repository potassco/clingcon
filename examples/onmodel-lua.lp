#script (lua)

function tostringTable(t) 
    ret   = ""
    comma = False
    for _, elem in pairs(t) do
        if comma then ret = ret .. ", "
        else          comma = true 
        end
        ret = ret .. tostring(elem)
    end
    return ret
end

function main(prg)
    on_model = function (m)
        print("theory")
        print("  positive: " .. tostringTable(m:symbols{extra=true}))
        print("  negative: " .. tostringTable(m:symbols{extra=true, complement=true}))
    end
    prg:ground({{"base", {}}})
    prg:solve{on_model=on_model}
end

#end.
#include <csp>.


{a}.
b :- a.

&dom{1..10} = x.

&show {x}.
