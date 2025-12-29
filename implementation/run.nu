# Create results directory if it doesn't exist
mkdir ./results/examples

# Iterate over files in ./examples
ls ./examples | each { |$it|
    # Get the filename without the path
    let filename = $it.name
    
    # Execute with 'exact' option
    print ("./results/" + $filename)
    let exact_output = ./bin/main.o exact $filename 3
    # Write the output to results folder
    echo $exact_output | save ("./results/" + $filename)

    # Execute with 'approx' option
    let approx_output = ./bin/main.o approx $filename 3
    # Write the output to results folder
    echo $approx_output | save ("./results/" + $filename) --append
}
