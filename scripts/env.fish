set directory (pwd)
function bplot
    echo "BookSim Plotter"
    python3 $directory/plotter/plotter.py $argv
end

function blaunch
    echo "BookSim Plotter"
    python3 $directory/booksim_launcher/launcher.py $argv
end
