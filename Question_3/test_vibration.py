import vibration

def run_tests():
    print("-- Testing Industrial Vibration Module --")
    
    # Sample Data
    data = [0.5, -1.2, 3.4, -0.8, 1.1, -2.5, 4.0, -1.5]
    print(f"Data: {data}\n")

    # Demonstrate required functions
    ptp = vibration.peak_to_peak(data)
    print(f"Peak-to-Peak:   {ptp:.4f} m/s²")
    
    rms = vibration.rms(data)
    print(f"RMS:            {rms:.4f} m/s²")
    
    std = vibration.std_dev(data)
    print(f"Std Deviation:  {std:.4f} m/s²")
    
    thresh = 1.0
    above = vibration.above_threshold(data, thresh)
    print(f"Above {thresh} m/s²: {above} readings")
    
    summary = vibration.summary(data)
    print(f"Summary:        {summary}\n")

    # Test edge cases & constraints
    print("-- Testing Edge Cases & Validation --")
    
    # Accept tuples
    tuple_data = (1.5, 2.5, 3.5)
    print(f"Tuple handling: {vibration.summary(tuple_data)}")
    
    # Empty List Handling
    try:
        vibration.rms([])
        print("FAIL: Did not catch empty list.")
    except ValueError as e:
        print(f"Empty list handled safely: {e}")

    # Single item for standard deviation 
    try:
        vibration.std_dev([5.0])
        print("FAIL: Did not catch n < 2 for std_dev.")
    except ValueError as e:
        print(f"N=1 std_dev handled safely: {e}")

    # Invalid type handling
    try:
        vibration.peak_to_peak(["string", 2.0])
        print("FAIL: Did not catch string input.")
    except TypeError as e:
        print(f"Invalid type handled safely: {e}")

if __name__ == "__main__":
    run_tests()
