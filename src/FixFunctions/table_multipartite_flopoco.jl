using JuMP, ArgParse, Gurobi; # CPLEX

const GRB_ENV = Gurobi.Env()

flopoco_dir = "" # Add the path to your flopoco dir here
dir = flopoco_dir * "src/FixFunctions/"


function parse_commandline()
    s = ArgParseSettings()

    @add_arg_table s begin
        "--f"
            help = "function written as the expression, in "
        "--msbIn"
            help = "Most significant bit of the input"
            arg_type = Int
        "--lsbIn"
            help = "Least significant bit of the input"
            arg_type = Int
        "--msbOut"
            help = "Most significant bit of the output"
            arg_type = Int
        "--lsbOut"
            help = "Least significant bit of the output"
            arg_type = Int
        "--correct"
            help = "Flag : correct rounding"
            action = :store_true
        "--FPGA"
            help = "Flag : Optimise the LUTs instead of the table size"
            action = :store_true
    end

    return parse_args(s)
end

function to_Int(x::Float64)
    return Int(round(x))
end

function cartesian_product(m, range)::Vector{Vector{Int64}}
    if m != 1
        B = cartesian_product(m-1, range);
        next_B = []
        for i in range
            append!(next_B, [collect(Base.Iterators.flatten([b, i])) for b in B])
        end
        return next_B
    else
        return [ [i] for i in range]
    end
end

function compute_beta(m, BETA)::Vector{Vector{Int64}}
    beta = []
    #iterator = cartesian_product(m, 2:(BETA - 2*(m-1)));
    iterator = cartesian_product(m, 2:3);
    for i in iterator
        if sum(i) == BETA
            append!(beta, [i]);
        end
    end
    return beta
end

struct decomposition
    alpha::Int
    beta::Vector{Int}
    gamma_min::Vector{Int}
    lsb_int_TIV::Int
    msb_int_TO::Vector{Int}
    cost::Float64
end

Base.:(==)(a::decomposition, b::decomposition) = (a.cost == b.cost);
Base.isless(a::decomposition, b::decomposition) = (a.cost < b.cost);
Base.show(io::IO, s::decomposition) = println(io, "alpha = $(s.alpha), beta = $(join(s.beta, ' ')), gamma_min = $(join(s.gamma_min, ' ')),  lsb_int_TIV = $(s.lsb_int_TIV), msb_int_TO = $(join(s.msb_int_TO, ' ')), cost = $(s.cost).");

function next_gamma(gamma::Array{Int}, m::Int, gamma_max::Int)
   for i in (m-1):-1:1
       if gamma[i] != gamma_max
           gamma[i] = gamma[i] +1
           for j in (i+1):(m-1)
               gamma[j] = gamma[i]
           end
           return 0
       end
   end
end

function all_gamma(gamma_min::Array{Int}, m::Int, gamma_max::Int, alpha::Int)
    gammas = Vector{Array{Int}}();
    append!(gammas, [gamma_min]);
    gamma = copy(gamma_min)
    gamma_max_vect = zeros(Int, m) .+ gamma_max;
    gamma_max_vect[m] = alpha;
    while gamma != gamma_max_vect
        for i in (m-1):-1:1
            if gamma[i] != gamma_max
                gamma[i] = gamma[i] +1
                for j in (i+1):(m-1)
                    gamma[j] = gamma[i]
                end
                append!(gammas, [copy(gamma)]);
                break;
            end
        end
    end
    return gammas;
end

function size_of_sol(FPGA::Bool, correct::Bool, alpha::Int, beta::Array{Int}, gamma::Array{Int}, msb_int_TO::Array{Int}, TIV_out_min::Int, lsb_out::Int, N::Int, bin_extra_bits::Int, g_bin_extra_bits::Int)
    m = length(beta);
    if FPGA
        use_xor = (gamma .+ beta .>= 7); #computed was 7
    else
        use_xor = fill(true, m); #always use xor (optimise for bits)
    end
    use_xor[m] = 0;

    g_faithful = 1 + floor(Int, log2(m-1));
    g_correct = 1 + (m-1)*ceil(Int, log2(N)-2);
    g_max = (correct) ? g_correct : g_faithful;
    lsb_min = lsb_out - g_max;
    TO_out_min = msb_int_TO .- (lsb_min + g_bin_extra_bits);

    if FPGA
        FPGA_N_TIV = (2^max(alpha-6, 0));
        FPGA_N_TO = (2 .^ max.(beta .+ gamma .- use_xor .-6, 0 ));
        min_size_of_sol = FPGA_N_TIV*TIV_out_min + sum(FPGA_N_TO .* TO_out_min) + 0.55*(TIV_out_min + sum(TO_out_min)) +  sum(use_xor .* (beta .- 1 .+ TO_out_min));
    else
        min_size_of_sol = 2^alpha * TIV_out_min + sum(2 .^ (gamma .+ beta .- use_xor) .* TO_out_min)
    end
    return min_size_of_sol;
end


function optim(FPGA::Bool, correct::Bool, m::Int, alpha::Int, beta::Array{Int}, gamma::Array{Int}, msb_in::Int, lsb_in::Int, msb_out::Int, lsb_out::Int, lsb_min::Int, lsb_int_TIV::Int, msb_int_TO::Array{Int}, y_low::Array{Int}, y_high::Array{Int}, bin_extra_bits::Int, g_bin_extra_bits::Int)
    msb_int_TIV = msb_out;

    #xor usage depend on the table
    if FPGA
        use_xor = (gamma .+ beta .>= 7); #computed was 7
    else
        use_xor = fill(true, m); #always use xor (optimise for bits)
    end
    # don't use xor for biggest TO (compression)
    use_xor[m] = 0;

    #other constants
    d = lsb_out - lsb_min;                #discarded bits
    N = msb_in-lsb_in+1;                  #number of values in function input
    N_TIV = 2^alpha;                      #number of values in TIV input
    N_TO = 2 .^ (beta+gamma .- use_xor ); #number of values in TO input with symmetry (or not) calculation
    FPGA_N_TIV = (2^max(alpha-6, 0));
    FPGA_N_TO = (2 .^ max.(beta+gamma .- use_xor .-6, 0 ));

    #Offsets of the beta parts
    offset = copy(beta);
    offset[1] = 0;
    for i in 1:(length(beta)-1)
        offset[i+1] = offset[i] + beta[i];
    end


    #Optimisation model
    model = Model(() -> Gurobi.Optimizer(GRB_ENV));
    #model = Model(CPLEX.Optimizer);
    set_silent(model)
    
    set_optimizer_attribute(model, "TimeLimit", 5*60);    
    set_optimizer_attribute(model, "IntFeasTol", 1e-9); 

    #set_optimizer_attribute(model, "CPX_PARAM_EPINT", 1e-10);
    #set_optimizer_attribute(model, "CPX_PARAM_EPINT", 0);
    #set_optimizer_attribute(model, "CPXPARAM_Sifting_Simplex", false);
    #set_optimizer_attribute(model, "CPXPARAM_Emphasis_MIP", 4);
    #set_optimizer_attribute(model, "CPX_PARAM_MIPORDTYPE", 2);
    #set_optimizer_attribute(model, "CPX_PARAM_PROBE", 3)

    # Table of initial Values
    #println("msb_int_TIV = ", msb_int_TIV, ", lsb_int_TIV = ", lsb_int_TIV, ", msb_TO = ", join(msb_int_TO, " "));
    @variable(model, 0 <= TIV_int[0:(N_TIV-1)] <= 2^(msb_int_TIV - lsb_int_TIV+1)-1, Int);
    lsb_TIV = max(lsb_int_TIV - bin_extra_bits, lsb_min);
    @variable(model, TIV_bin_lsb[0:(N_TIV-1), lsb_TIV:(lsb_int_TIV-1)], Bin);
    @variable(model, TIV[0:(N_TIV-1)], Int);
    for i in 0:(N_TIV-1)
        @constraint(model, TIV[i] == sum(2^(j-lsb_min) * TIV_bin_lsb[i, j] for j in lsb_TIV:(lsb_int_TIV-1)) + TIV_int[i] * 2^(lsb_int_TIV-lsb_min))
    end

    # Table of offsets, m different tables
    TO_int = [];
    TO_bin_msb = [];
    TO_bin_lsb = [];
    TO = [];
    msb_TO = min.(msb_int_TO .+ bin_extra_bits, msb_out);
    lsb_int_TO = min(minimum(msb_int_TO), lsb_min + g_bin_extra_bits);
    for i in 1:m
        push!(TO_int, @variable(model, [0:(N_TO[i]-1)], integer= true, lower_bound = 0, upper_bound = 2^(msb_int_TO[i] - lsb_int_TO+1)-1, base_name = string("TO", i)*"_int"));
        push!(TO_bin_msb, @variable(model, [0:(N_TO[i]-1), (msb_int_TO[i]+1):msb_TO[i]], binary= true, base_name = string("TO", i)*"_bin_msb"));
        push!(TO_bin_lsb, @variable(model, [0:(N_TO[i]-1), lsb_min:(lsb_int_TO-1)], binary= true, base_name = string("TO", i)*"_bin_lsb"));
        push!(TO, @variable(model, [0:(N_TO[i]-1)], integer= true, base_name = string("TO", i)));
    end
    
    for t in 1:m
        for i in 0:(N_TO[t]-1)
            @constraint(model, TO[t][i] == sum( 2^(j-lsb_min) * TO_bin_lsb[t][i, j] for j in lsb_min:(lsb_int_TO-1)) + TO_int[t][i] * 2^(lsb_int_TO-lsb_min) + sum( 2^(j-lsb_min) * TO_bin_msb[t][i, j] for j in (msb_int_TO[t]+1):msb_TO[t]))
        end
    end


    #bunch of variables needed to measure table size
    # Size of TIV
    @variable(model, mu_TIV[0:(N_TIV-1), lsb_TIV:(lsb_int_TIV-1)], Bin);
    @variable(model, mu_max_TIV[lsb_TIV:(lsb_int_TIV-1)], Bin);
    @variable(model, trailing_zeros_TIV >= 0, Int);
    @variable(model, size_TIV >= 0, Int);

    # Size of TO
    mu_TO = [];
    mu_max_TO = [];
    nu_TO = [];
    #@variable(model, mu_TO[0:(N_TO-1), lsb_min:msb_TO], Bin)
    for i in 1:m
        push!(mu_TO, @variable(model, [0:(N_TO[i]-1), lsb_min:(lsb_int_TO-1)], binary= true, base_name = string("mu_TO", i)));
        push!(mu_max_TO, @variable(model, [lsb_min:(lsb_int_TO-1)], binary= true, base_name = string("mu_max_TO", i)));
        push!(nu_TO, @variable(model, [0:(N_TO[i]-1), (msb_int_TO[i]+1):msb_TO[i]], binary= true, base_name = string("nu_TO", i)));
    end
    @variable(model, trailing_zeros_TO[1:m] >= 0, Int);
    @variable(model, leading_zeros_TO[1:m] >= 0, Int);
    @variable(model, size_TO[1:m] >= 0, Int);


    # Check that correctly rounded
    for i in 0:(2^N -1)
        index_TIV = floor(Int, i/2^(N-alpha));

        # Regarder si les TO ont un 0 en msb ie il faut soustraire au début (on fait que sur du croissant, ça devrait passer)
        xor_TO = mod.(floor.(Int, i ./2 .^(beta+offset .- 1)), 2);
        beta_word = [mod(floor(Int, i/2^offset[j]), 0:(2^(beta[j]-1) -1)) for j in 1:m];
        xor_word = 2 .^ (beta .- 1) .- 1;
        index_TO_xor = floor.(Int, i ./ 2 .^ (N .- gamma)) .* (2 .^ (beta .- 1)) + [ if (xor_TO[t]==0) beta_word[t] ⊻ xor_word[t] else beta_word[t] end for t in 1:m];
        index_TO_no_xor = floor.(Int, i ./ 2 .^ (N .- gamma)) .* (2 .^ beta) + [mod(floor(Int, i/2^offset[j]), 0:(2^beta[j] -1)) for j in 1:m];

        index_TO = use_xor .* index_TO_xor .+ (1 .- use_xor) .* index_TO_no_xor;
        is_sub_xor = xor_TO .*2 .- 1; #substraction when xor_TO is 0
        is_sub = is_sub_xor .* use_xor .+ (1 .- use_xor);
        is_xor = (1 .- xor_TO) .* use_xor;

        #=println(TIV_bin_lsb);
        println(TIV_int);
        println(is_sub);
        println(TO_bin_lsb);
        println(TO_bin_msb);
        println(index_TO);
        println(msb_int_TO);
        println(msb_TO);
        println(TO_int);
        println(is_xor);
        println(mu_max_TO);=#

        @constraint(model, TIV[index_TIV] #TIV
                           + sum( is_sub[t] * TO[t][index_TO[t]] for t in 1:m)
                           - sum(is_xor[t] * (1 + sum(2^(j-lsb_min) * mu_max_TO[t][j] for j in lsb_min:(lsb_int_TO-1))) for t in 1:m) # xor compensation
                           >=
                           y_low[i+1] * 2^d #y
        , base_name="lower_bound");
        @constraint(model, TIV[index_TIV] #TIV
                           + sum( is_sub[t] * TO[t][index_TO[t]] for t in 1:m)
                           - sum(is_xor[t] * (1 + sum(2^(j-lsb_min) * mu_max_TO[t][j] for j in lsb_min:(lsb_int_TO-1))) for t in 1:m) # xor compensation
                           <=
                           y_high[i+1]*2^d + 2^d -1 #y + 2^d -1
        , base_name="upper_bound");

        #check fits in output
        @constraint(model, TIV[index_TIV] #TIV
                           + sum( is_sub[t] * TO[t][index_TO[t]] for t in 1:m)
                           - sum(is_xor[t] * (1 + sum(2^(j-lsb_min) * mu_max_TO[t][j] for j in lsb_min:(lsb_int_TO-1))) for t in 1:m) # xor compensation
                           <=
                           2^(msb_out - lsb_min + 1) -1
        , base_name="fits_in_output");
    end

    
    # help the ILP, constraint the TIV values
    # function monotone. Compute difference between edges of interval
    for i in 0:(2^(alpha)-1) #for each TIV value
        #edges
        interval_beginning = i*2^(N-alpha);
        interval_end = (i+1)*2^(N-alpha)-1;
        interval_middle = i*2^(N-alpha) + 2^(N-alpha-1);
        y_diff = y_high[interval_end + 1]* 2^d - y_low[interval_beginning + 1] * 2^d;
        @constraint(model, y_low[interval_middle + 1]  * 2^d - 2*y_diff - 1 <= TIV[i] <= y_high[interval_middle + 1] * 2^d + 2*y_diff + 1);
        #println(interval_beginning, ' ', interval_end, ' ', interval_middle, ' ', y_diff, ' ', sum(2^(j-lsb_min) * y_low[interval_middle,j] for j in lsb_out:msb_out) - 2*y_diff - 1, ' ', sum(2^(j-lsb_min) * y_high[interval_middle, j] for j in lsb_out:msb_out) + 2*y_diff + 1);
    end;
    

    # Measure size of tables (ILP dark magic explained in the article)
    # mu for end of table, nu for beginning

    for i in 0:(N_TIV-1) #for every number in the table
        # Compute the trailing 0 at the end of that number
        for j in lsb_TIV:(lsb_int_TIV-1) #for every bit in the number
            @constraint(model, 1 <= mu_TIV[i, j] + sum(TIV_bin_lsb[i, k] for k in lsb_TIV:j));
            for k in lsb_TIV:j
                @constraint(model, 1 >= mu_TIV[i, j] + TIV_bin_lsb[i, k]);
            end
        end
        @constraint(model, trailing_zeros_TIV <= sum(mu_TIV[i, j] for j in lsb_TIV:(lsb_int_TIV-1))); #total number of trailing zeros can't be bigger than the trailing zeros here
    end
    # Add a constraint for the table size bigger than size of that number (msb_tiv - lsb_min - number of trailing zeros - number of leading zeros)
    @constraint(model, size_TIV == msb_int_TIV - lsb_TIV - trailing_zeros_TIV + 1);


    for t in 1:m #for every table
        for i in 0:(N_TO[t]-1) #for every number in the table
            # Compute the trailing 0 at the end of that number
            for j in lsb_min:(lsb_int_TO-1) #for every bit in the number
                @constraint(model, mu_TO[t][i, j] >= mu_max_TO[t][j]);
                @constraint(model, 1 <= mu_TO[t][i, j] + sum(TO_bin_lsb[t][i, k] for k in lsb_min:j));
                for k in lsb_min:j
                    @constraint(model, 1 >= mu_TO[t][i, j] + TO_bin_lsb[t][i, k]);
                end
            end
            # Compute the leading 0 at the beginning of that number
            for j in msb_TO[t]:-1:(msb_int_TO[t]+1) #for every bit in the number
                @constraint(model, 1 <= nu_TO[t][i, j] + sum(TO_bin_msb[t][i, k] for k in (msb_TO[t]):-1:j));
                for k in msb_TO[t]:-1:j
                    @constraint(model, 1 >= nu_TO[t][i, j] + TO_bin_msb[t][i, k]);
                end
            end
            @constraint(model, leading_zeros_TO[t] <= sum(nu_TO[t][i, j] for j in (msb_int_TO[t]+1):msb_TO[t]));
        end
        #mu_max are decreasing so we're sure we have the right xor
        for j in lsb_min:(lsb_int_TO-2)
            @constraint(model, mu_max_TO[t][j] >= mu_max_TO[t][j+1]);
        end            
        @constraint(model, trailing_zeros_TO[t] == sum(mu_max_TO[t][j] for j in lsb_min:(lsb_int_TO-1)));
    end

    # Add a constraint for the table size bigger than size of that number (msb_tiv - lsb_min - number of trailing zeros - number of leading zeros)
    @constraint(model, size_TO .== msb_TO .- lsb_min .- trailing_zeros_TO .- leading_zeros_TO + 1);


    # Help the ILP
    # TIV is increasing
    # For a given gamma, TO entries are also increasing
    for i in 0:(N_TIV-2)
        @constraint(model, TIV[i] <= TIV[i+1]);
    end
    for t in 1:m
        for c in 0:(2^gamma[t]-1)
            for b in 0:(2^(beta[t] - use_xor[t]) -2)
                i = c * 2 ^ (beta[t] - use_xor[t]) + b
                if i+1 >= N_TO[t]
                    println("gamma = ", gamma[t], ", c = " ,c, ", b = " , b, ", i = ", i);
                end
                @constraint(model, TO[t][i] <= TO[t][i+1]);
            end
        end
    end

    # Objective : minimise the table size

    if FPGA
        # FPGA objective
        @objective(model, Min, FPGA_N_TIV*size_TIV + sum(FPGA_N_TO .* size_TO) + 0.55*(size_TIV + sum(size_TO)) +  0.5 * sum(use_xor .* (beta .- 1 .+ size_TO)));
    else
        @objective(model, Min, N_TIV*size_TIV + sum(N_TO .* size_TO));
    end
    
    #Optimiser
    optimize!(model);

    if has_values(model) == false
        return 0
    end
    
    printfile = "funoutput.txt"
    ffo = open(dir * printfile, "w")
    #Print les trucs
    new_msb_TIV = msb_int_TIV;
    TIV_size = value(size_TIV);
    TIV_area = N_TIV * TIV_size;
    TO_size = value.(size_TO);
    new_msb_TO = msb_TO .- value.(leading_zeros_TO);
    TO_area = N_TO .* TO_size;

    println(ffo, "Output size of TIV : $(TIV_size) for msb $(new_msb_TIV) (Number of stored bits $(TIV_area) = $(TIV_size) × 2^$(alpha))")
    for t in m:-1:1
        println(ffo, "Output size of TO", t, " : $(TO_size[t]) for msb $(new_msb_TO[t]) (Number of stored bits $(TO_area[t]) = $(TO_size[t]) × 2^$(gamma[t]+beta[t]-use_xor[t])) $((use_xor[t]) ? "using symmetry" : "without using symmetry")")
    end
    println(ffo, "Total table size : $(TIV_area + sum(TO_area))")
    if FPGA
        println(ffo, "LUT number estimate : $(objective_value(model))")
    end
    println(ffo, "Naive table without compression : $((msb_out-lsb_out+1) * 2^N) = $(msb_out-lsb_out+1) × 2^$(N)") 

    # Representation of the positions (only with msbs < 0)
    println(ffo, "Alignment drawing")
    global s = "0"
    for i in 1:(-new_msb_TIV-1)
        global s = s*"."
    end
    for i in 1:TIV_size
        global s = s*"*"
    end
    println(ffo, s)
    for t in m:-1:1
        global s = "0"
        for i in 1:(-new_msb_TO[t]-1)
            global s = s*"."
        end
        for i in 1:TO_size[t]
            global s = s*"*"
        end
        println(ffo, s)
    end
    global s = " "
    for i in lsb_min:-1
        global s = s * "_"
    end
    println(ffo, s)
    global s = "0"
    for i in 1:(-msb_out-1)
        global s = s*"."
    end
    for i in lsb_out:msb_out
        global s = s*"*"
    end
    global s = s * "|"
    println(ffo, s)
    close(ffo)

    #save data from tables
    outfile = "tables.data"
    ff = open(dir * outfile, "w")
    new_lsb_TIV = to_Int(new_msb_TIV-TIV_size+1)
    new_lsb_TO =  to_Int.(new_msb_TO .- TO_size .+ 1)
    println(ff, m, " ", alpha, "\n", join(beta, ' '), "\n", join(gamma, ' ')); #m, alpha, beta, gamma
    println(ff, join( Int.(use_xor), ' ')) #use_xor for TOs
    println(ff, new_msb_TIV, " ", new_lsb_TIV) #size TIV
    println(ff, join( to_Int.(new_msb_TO), ' ')) #msb_TO
    println(ff, join( new_lsb_TO, ' ')) #lsb_TO

    for i in 0:(N_TIV-1)
        if (new_lsb_TIV == lsb_int_TIV)
             bin_lsb_TIV = 0;
        else
            bin_lsb_TIV = sum(2^(j-new_lsb_TIV) * to_Int(value(TIV_bin_lsb[i, j])) for j in new_lsb_TIV:(lsb_int_TIV-1));
        end
        val_TIV = bin_lsb_TIV + to_Int(value(TIV_int[i])) * 2^(lsb_int_TIV - new_lsb_TIV)
        println(ff, val_TIV)
    end
    for t in 1:m
        for i in 0:(N_TO[t]-1)
            if (new_lsb_TO[t] == lsb_int_TO)
                bin_lsb_TO = 0
            else
                bin_lsb_TO = sum( 2^(j-new_lsb_TO[t]) * to_Int(value(TO_bin_lsb[t][i, j])) for j in new_lsb_TO[t]:(lsb_int_TO-1));
            end
            if (msb_int_TO[t] == msb_TO[t])
                bin_msb_TO = 0
            else
                bin_msb_TO = sum( 2^(j-new_lsb_TO[t]) * to_Int(value(TO_bin_msb[t][i, j])) for j in (msb_int_TO[t]+1):msb_TO[t]);
            end
            val_TO =  bin_lsb_TO + to_Int(value(TO_int[t][i])) * 2^(lsb_int_TO-new_lsb_TO[t]) + bin_msb_TO
            println(ff, val_TO)
        end
    end
    close(ff)



    outfile = "g_data.data";
    fg = open(dir * outfile, "a");
    println(fg, "For m=$(m), alpha=$(alpha), beta=[$(join(beta, ','))], gamma=[$(join(gamma, ','))], with size $(objective_value(model)) : the optimal is g=$(lsb_out - minimum(new_lsb_TO)).");
    close(fg)

    return objective_value(model);
end

function find_param(FPGA::Bool, correct::Bool, msb_in::Int, lsb_in::Int, msb_out::Int, lsb_out::Int, y_low::Array{Int}, y_high::Array{Int}, bin_extra_bits::Int, g_bin_extra_bits::Int)
    
    N = msb_in-lsb_in+1

    if FPGA
        min_text = "lut estimate"
        flopoco_size = 350
        use_flopoco_size = false
        min_size = (use_flopoco_size) ? flopoco_size : (2^max(N-6, 0)) * (msb_out-lsb_out+1);
    else
        min_text = "table size"
        min_size = 2^N * (msb_out-lsb_out+1);
    end

    decompositions = Vector{decomposition}();

    gamma_min = Dict{Tuple{Int,Int}, Int}();
    msb_TO = Dict{Tuple{Int, Int}, Int}();

    META_VALUES = Vector{Vector{Int}}();

    for m in 1:N÷4
        for alpha_beta in (N÷2):(N-2*m) # on pourrait finir avant N-2*m car certaines compressions sont nulles
            compressions = [(alpha, beta_m) for alpha=((alpha_beta+1)÷2):alpha_beta, beta_m=3:-1:2 if alpha+beta_m==alpha_beta];
            if length(compressions) == 0
                continue
            end
            for (a, b) in compressions
                msb_TO[((N-alpha_beta), b)] = 0;
            end;
            BETAS = compute_beta(m, N-alpha_beta);
            for beta_vec in BETAS
                for i in 1:m
                    gamma_min[(sum(beta_vec[1:i-1]), beta_vec[i])] = 0;
                    msb_TO[(sum(beta_vec[1:i-1]), beta_vec[i])] = 0;
                end
            end
            for (B, C) in Base.Iterators.product(BETAS, compressions)
                append!(META_VALUES, [vcat([C[1]], B, [C[2]])])
            end
        end
    end

    # computing the gammas
    for (key, values) in gamma_min
        p = key[1]
        beta = key[2]
        gamma_max = N-p-beta
        for gamma in 0:gamma_max
            err = []
            for c in 0:(2^gamma-1)
                beginning_slope = y_high[c*2^(N-gamma-p) + 2^(beta+p)-1+1] - y_low[c*2^(N-gamma-p)+1]
                end_slope = y_high[(c+1)*2^(N-gamma-p)-1+1] - y_low[(c+1)*2^(N-gamma-p) - 2^(beta+p)+1]
                append!(err, abs(beginning_slope-end_slope))
            end
            if maximum(err) <= 2 - correct # 2 for faithful, 1 for correct
                gamma_min[key] = max(gamma-1, 0);
                break
            end
        end
    end

    #computing the msbs
    for (key, values) in msb_TO
        p = key[1]
        beta = key[2]
        max_interval = maximum([(y_high[xsup * 2^(beta+p) + (2^beta-1)*2^p + xinf + 1] - y_low[xsup * 2^(beta+p) + 0 + xinf + 1]) for xsup=0:(2^(N-beta-p)-1) for xinf=0:(2^p-1)])
        msb_TO[key] = lsb_out + ceil(Int, log2(max_interval))
    end 

    # putting this in order
    for d in META_VALUES
        m = length(d) - 1



        alpha = d[1]
        beta = d[2:m+1]
        gamma_max = alpha + beta[m]
        offset = zeros(Int, m);
        for k in 1:(m-1)
            offset[k+1] = offset[k] + beta[k];
        end
        gamma = zeros(Int, m);
        msb_int_TO = zeros(Int, m);
        gamma[1] = gamma_min[(offset[1], beta[1])]
        gamma[m] = alpha
        for t in 1:m #use the right precomputed starting gamma, and compute msb_TO (could be precomputed too)
            p = offset[t]
            b = beta[t]
            if t != m && t != 1
                gamma[t] = max(gamma_min[(p, b)], gamma[t-1]) # keep increasing gammas
            end
            msb_int_TO[t] = msb_TO[(p, b)];
        end
        lsb_int_TIV = min(msb_int_TO[m], msb_out);

        if maximum(gamma) > gamma_max # the solution can't be feasable
            continue
        end
        
        TIV_out_min = msb_out - lsb_int_TIV +1;

        # Compute the correct gamma
        gammas = all_gamma(gamma, m, gamma_max, alpha);
        if FPGA
            gamma_FPGA_sizes = map((gam -> (gam,  size_of_sol(true, correct, alpha, beta, gam, msb_int_TO, TIV_out_min, lsb_out, N, bin_extra_bits, g_bin_extra_bits))), gammas);
            sort!(gamma_FPGA_sizes, by = x -> x[2], alg=InsertionSort);
            cost = gamma_FPGA_sizes[1][2];
        else
            gamma_bits_sizes = map((gam -> (gam,  size_of_sol(false, correct, alpha, beta, gam, msb_int_TO, TIV_out_min, lsb_out, N, bin_extra_bits, g_bin_extra_bits))), gammas);
            sort!(gamma_bits_sizes, by = x -> x[2], alg=InsertionSort);
            cost = gamma_bits_sizes[1][2];
        end        
        append!(decompositions, [decomposition(alpha, beta, gamma, lsb_int_TIV, msb_int_TO, cost)]);
    end

    sorted_decompositions = sort(decompositions);
    #=
    sorted_decompositions2 = Vector{decomposition}();
    
    for d in sorted_decompositions
        if length(sorted_decompositions2) == 0 && d.beta != [5, 4]
            continue;
        end
        append!(sorted_decompositions2, [d]);
    end
    println(sorted_decompositions2) =#
        
    min_m = 0;
    min_alpha = 0;
    min_beta = [];
    min_gamma = [];
    println("Limit for not trying ILP is $(min_text) : $(min_size)");

    # Going through the decomposition

    for d in sorted_decompositions
        m = length(d.beta)

        alpha = d.alpha
        beta = d.beta
        offset = zeros(Int, m);
        for k in 1:(m-1)
            offset[k+1] = offset[k] + beta[k];
        end
        gamma = d.gamma_min;
        gamma_max = alpha + beta[m];
        msb_int_TO = d.msb_int_TO;
        lsb_int_TIV = d.lsb_int_TIV;

        # with precomputation etc, and then int thing, we can compute the min number of luts of our solution
        # since we have a min size for all the wout of the tables
        # this way we can disqualify some solutions we know can't be better
        TIV_out_min = msb_out - lsb_int_TIV +1;

        # Compute the correct gamma
        gammas = all_gamma(gamma, m, gamma_max, alpha);
        gamma_sizes = map((gam -> (gam,  size_of_sol(FPGA, correct, alpha, beta, gam, msb_int_TO, TIV_out_min, lsb_out, N, bin_extra_bits, g_bin_extra_bits))), gammas);
        sort!(gamma_sizes, by = x -> x[2], alg=InsertionSort); #insertion sort because n is still small and we want a stable sorting algo
        #filter!(x -> x[2] <= min_size, gamma_sizes); #remove all the solutions that are too big

        
        #only do the worse gamma test while we don't have a solution, this is a starting problem
        # to help the "faithful" problem, maybe use the worse gamma that is still smaller than the flopoco solution ?
        if min_m == 0
            #=
            worse_gamma = last(gamma_sizes)[1];
            min_size_of_sol = last(gamma_sizes)[2];
            =#
            
            worse_gamma = zeros(Int, m) .+ (alpha + beta[m]);
            worse_gamma[m] = alpha;
            find_size = filter(x -> x[1] == worse_gamma, gamma_sizes);
            min_size_of_sol = find_size[1][2];
            
           if FPGA
                use_xor = (worse_gamma .+ beta .>= 7); #computed was 7
            else
                use_xor = fill(true, m); #always use xor (optimise for bits)
            end
            use_xor[m] = 0;

            g_faithful = 1 + floor(Int, log2(m-1));
            g_correct = 1 + (m-1)*ceil(Int, log2(N)-2);
            g_max = (correct) ? g_correct : g_faithful;
            println("g = ", g_max);
            lsb_min = lsb_out - g_max;

            println("Starting new beta with m=", m, ", alpha=", alpha, ", beta=[", join(beta, ','), "]. Starting test for worse gamma=[", join(worse_gamma, ','), "], with minimum cost $(min_text) : $(min_size_of_sol).");
            worse_gamma_table_size = optim(FPGA, correct, m, alpha, beta, worse_gamma, msb_in, lsb_in, msb_out, lsb_out, lsb_min, lsb_int_TIV, msb_int_TO, y_low, y_high, bin_extra_bits, g_bin_extra_bits);
            if worse_gamma_table_size != 0
                println("Result found for ", min_text, " : ", worse_gamma_table_size, ", this beta is feasible, starting optimisation.");
            else
                println("No result found, this beta is unfeasible, skipping optimisation.");
                continue;
            end
        end



        for (gamma, min_size_of_sol) in gamma_sizes
            if min_size_of_sol >= min_size #Already bigger than what we already have
                println("Abandon computing for m=", m, ", alpha=", alpha, ", beta=[", join(beta, ','), "], gamma=[", join(gamma, ','), "]. Minimum $(min_text) $(min_size_of_sol) is larger than current minimum $(min_size).");
                break; #used to be continue, now break since gammas are now in order of cost
            end
            if FPGA
                use_xor = (gamma .+ beta .>= 7); #computed was 7
            else
                use_xor = fill(true, m); #always use xor (optimise for bits)
            end
            use_xor[m] = 0;
            g_faithful = 1 + floor(Int, log2(m-1));
            g_correct = 1 + (m-1)*ceil(Int, log2(N)-2);
            g_max = (correct) ? g_correct : g_faithful;
            println("g = ", g_max);
            lsb_min = lsb_out - g_max;

            println("Computing tables for m=", m, ", alpha=", alpha, ", beta=[", join(beta, ','), "], gamma=[", join(gamma, ','), "]. Minimum $(min_text) : $(min_size_of_sol).");
            table_size = optim(FPGA, correct, m, alpha, beta, gamma, msb_in, lsb_in, msb_out, lsb_out, lsb_min, lsb_int_TIV, msb_int_TO, y_low, y_high, bin_extra_bits, g_bin_extra_bits);
            if table_size != 0
                println("Result found for ", min_text, " : ", table_size);
                last_m_found = m;
                if table_size < min_size
                    min_size = table_size;
                    min_m = m;
                    min_alpha = alpha;
                    min_beta = copy(beta);
                    min_gamma = copy(gamma);
                    mv(dir * "tables.data", dir * "min_tables.data", force=true);
                    mv(dir * "funoutput.txt", dir * "min_funoutput.txt", force=true);
                    println("New limit for not trying ILP is $(min_text) : $(min_size)");
                end
                #break; #no need to continue with this gamma (since rest are worse sizes (and keeps the same order rn if the sorting algorithm is stable))
            end
        end
    end
    if min_m == 0
        println("No approximation better than naive table ($(min_text) : $(min_size)) found");
        return 1;
    end
    println("Minimum size found was for m=", min_m, ", alpha=", min_alpha, ", beta=[", join(min_beta, ','), "], gamma=[", join(min_gamma, ','), "], with ", min_text, " ", min_size);
    mv(dir * "min_tables.data", dir * "tables.data", force=true);
    mv(dir * "min_funoutput.txt", dir * "funoutput.txt", force=true);
    ffo = open(dir * "funoutput.txt", "r");
    for lines in readlines(ffo)
        println(lines) ;      
    end
    close(ffo);
    return 0;
end


function main()
    parsed_args = parse_commandline();
    println("Parsed args:")
    for (arg,val) in parsed_args
        println("  $arg  =>  $val")
    end

    # Make function f exist
    function f(a::Float64)
        d= @eval x -> $(Meta.parse(parsed_args["f"]))
        return Base.invokelatest(d, a)
    end

    msb_in = parsed_args["msbIn"]
    lsb_in = parsed_args["lsbIn"]
    msb_out = parsed_args["msbOut"]
    lsb_out = parsed_args["lsbOut"]
    correct = parsed_args["correct"]
    FPGA = parsed_args["FPGA"]

    bin_extra_bits = 2
    g_bin_extra_bits = 1 + correct

    N = msb_in-lsb_in+1;

    recompute = false
    if recompute
        print("Filling the table of expected results ... ");
        y = zeros(Int, 2^N);
        y_low = zeros(Int, 2^N);
        y_high = zeros(Int, 2^N);

        if correct
            for x in 0:(2^N-1)
                x_float = x/2^(-lsb_in); #if lsb_in is positive, this will need to be 2.0
                y_float = f(x_float);
                y[x+1] = convert(Int64, round(y_float * 2^(-lsb_out), digits=0));
            end
            y_high = y;
            y_low = y;
        else
            for x in 0:(2^N-1)
                x_float = x/2^(-lsb_in);
                y_float = f(x_float);
                y_low[x+1] = convert(Int64, floor(y_float * 2^(-lsb_out), digits=0));
                y_high[x+1] = convert(Int64, ceil(y_float * 2^(-lsb_out), digits=0));
            end
        end
        println("Done");
        outfile = "g_data.data";
        fg = open(dir * outfile, "w");
        close(fg)
        find_param(FPGA, correct, msb_in, lsb_in, msb_out, lsb_out, y_low, y_high, bin_extra_bits, g_bin_extra_bits);
        #=m = 3;
        alpha=3;
        beta = [2, 3, 2]
        gamma = [5, 5, 3]
        lsb_int_TIV = -1;
        msb_int_TO = [-8,-5,-3];
        lsb_min = lsb_out - 5
        table_size = optim(FPGA, correct, m, alpha, beta, gamma, msb_in, lsb_in, msb_out, lsb_out, lsb_min, lsb_int_TIV, msb_int_TO, y_low, y_high);
        println(table_size)
        =#
    end
end

main()
