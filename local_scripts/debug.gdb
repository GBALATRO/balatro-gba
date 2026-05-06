set logging file gdb-output.txt
set logging overwrite off
set breakpoint pending on
set logging enabled on

define print_ram
    set $addr = $arg0
    set $len = $arg1
    x/$lenxb $addr
end

define print_frame_tick
    enable 1
    continue
    set $i = 0
    while $i < 2
        next
        set logging enabled on
        print frame_time
        set logging enabled off
        set $j = 0
        while $j < 6
            continue
            set $j = $j + 1
        end
        set $i = $i + 1
    end
end

define print_list
    p $arg0 
end

set prompt [gbagdb]>

set style enabled on

set history save on
set print pretty on

b profile_timer_breakpoint

disable 1