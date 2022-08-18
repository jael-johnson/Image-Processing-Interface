#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
using namespace std;

//***************************************************************************************************//
//                               Below code in this section was provided from professor              //
//***************************************************************************************************//

// Pixel structure
struct Pixel
{
    // Red, green, blue color values
    int red;
    int green;
    int blue;
};

/**
 * Gets an integer from a binary stream.
 * Helper function for read_image()
 * @param stream the stream
 * @param offset the offset at which to read the integer
 * @param bytes  the number of bytes to read
 * @return the integer starting at the given offset
 */ 
int get_int(fstream& stream, int offset, int bytes)
{
    stream.seekg(offset);
    int result = 0;
    int base = 1;
    for (int i = 0; i < bytes; i++)
    {   
        result = result + stream.get() * base;
        base = base * 256;
    }
    return result;
}

/**
 * Reads the BMP image specified and returns the resulting image as a vector
 * @param filename BMP image filename
 * @return the image as a vector of vector of Pixels
 */
vector<vector<Pixel>> read_image(string filename)
{
    // Open the binary file
    fstream stream;
    stream.open(filename, ios::in | ios::binary);

    // Get the image properties
    int file_size = get_int(stream, 2, 4);
    int start = get_int(stream, 10, 4);
    int width = get_int(stream, 18, 4);
    int height = get_int(stream, 22, 4);
    int bits_per_pixel = get_int(stream, 28, 2);

    // Scan lines must occupy multiples of four bytes
    int scanline_size = width * (bits_per_pixel / 8);
    int padding = 0;
    if (scanline_size % 4 != 0)
    {
        padding = 4 - scanline_size % 4;
    }

    // Return empty vector if this is not a valid image
    if (file_size != start + (scanline_size + padding) * height)
    {
        return {};
    }

    // Create a vector the size of the input image
    vector<vector<Pixel>> image(height, vector<Pixel> (width));

    int pos = start;
    // For each row, starting from the last row to the first
    // Note: BMP files store pixels from bottom to top
    for (int i = height - 1; i >= 0; i--)
    {
        // For each column
        for (int j = 0; j < width; j++)
        {
            // Go to the pixel position
            stream.seekg(pos);

            // Save the pixel values to the image vector
            // Note: BMP files store pixels in blue, green, red order
            image[i][j].blue = stream.get();
            image[i][j].green = stream.get();
            image[i][j].red = stream.get();

            // We are ignoring the alpha channel if there is one

            // Advance the position to the next pixel
            pos = pos + (bits_per_pixel / 8);
        }

        // Skip the padding at the end of each row
        stream.seekg(padding, ios::cur);
        pos = pos + padding;
    }

    // Close the stream and return the image vector
    stream.close();
    return image;
}

/**
 * Sets a value to the char array starting at the offset using the size
 * specified by the bytes.
 * This is a helper function for write_image()
 * @param arr    Array to set values for
 * @param offset Starting index offset
 * @param bytes  Number of bytes to set
 * @param value  Value to set
 * @return nothing
 */
void set_bytes(unsigned char arr[], int offset, int bytes, int value)
{
    for (int i = 0; i < bytes; i++)
    {
        arr[offset+i] = (unsigned char)(value>>(i*8));
    }
}

/**
 * Write the input image to a BMP file name specified
 * @param filename The BMP file name to save the image to
 * @param image    The input image to save
 * @return True if successful and false otherwise
 */
bool write_image(string filename, const vector<vector<Pixel>>& image)
{
    // Get the image width and height in pixels
    int width_pixels = image[0].size();
    int height_pixels = image.size();

    // Calculate the width in bytes incorporating padding (4 byte alignment)
    int width_bytes = width_pixels * 3;
    int padding_bytes = 0;
    padding_bytes = (4 - width_bytes % 4) % 4;
    width_bytes = width_bytes + padding_bytes;

    // Pixel array size in bytes, including padding
    int array_bytes = width_bytes * height_pixels;

    // Open a file stream for writing to a binary file
    fstream stream;
    stream.open(filename, ios::out | ios::binary);

    // If there was a problem opening the file, return false
    if (!stream.is_open())
    {
        return false;
    }

    // Create the BMP and DIB Headers
    const int BMP_HEADER_SIZE = 14;
    const int DIB_HEADER_SIZE = 40;
    unsigned char bmp_header[BMP_HEADER_SIZE] = {0};
    unsigned char dib_header[DIB_HEADER_SIZE] = {0};

    // BMP Header
    set_bytes(bmp_header,  0, 1, 'B');              // ID field
    set_bytes(bmp_header,  1, 1, 'M');              // ID field
    set_bytes(bmp_header,  2, 4, BMP_HEADER_SIZE+DIB_HEADER_SIZE+array_bytes); // Size of BMP file
    set_bytes(bmp_header,  6, 2, 0);                // Reserved
    set_bytes(bmp_header,  8, 2, 0);                // Reserved
    set_bytes(bmp_header, 10, 4, BMP_HEADER_SIZE+DIB_HEADER_SIZE); // Pixel array offset

    // DIB Header
    set_bytes(dib_header,  0, 4, DIB_HEADER_SIZE);  // DIB header size
    set_bytes(dib_header,  4, 4, width_pixels);     // Width of bitmap in pixels
    set_bytes(dib_header,  8, 4, height_pixels);    // Height of bitmap in pixels
    set_bytes(dib_header, 12, 2, 1);                // Number of color planes
    set_bytes(dib_header, 14, 2, 24);               // Number of bits per pixel
    set_bytes(dib_header, 16, 4, 0);                // Compression method (0=BI_RGB)
    set_bytes(dib_header, 20, 4, array_bytes);      // Size of raw bitmap data (including padding)                     
    set_bytes(dib_header, 24, 4, 2835);             // Print resolution of image (2835 pixels/meter)
    set_bytes(dib_header, 28, 4, 2835);             // Print resolution of image (2835 pixels/meter)
    set_bytes(dib_header, 32, 4, 0);                // Number of colors in palette
    set_bytes(dib_header, 36, 4, 0);                // Number of important colors

    // Write the BMP and DIB Headers to the file
    stream.write((char*)bmp_header, sizeof(bmp_header));
    stream.write((char*)dib_header, sizeof(dib_header));

    // Initialize pixel and padding
    unsigned char pixel[3] = {0};
    unsigned char padding[3] = {0};

    // Pixel Array (Left to right, bottom to top, with padding)
    for (int h = height_pixels - 1; h >= 0; h--)
    {
        for (int w = 0; w < width_pixels; w++)
        {
            // Write the pixel (Blue, Green, Red)
            pixel[0] = image[h][w].blue;
            pixel[1] = image[h][w].green;
            pixel[2] = image[h][w].red;
            stream.write((char*)pixel, 3);
        }
        // Write the padding bytes
        stream.write((char *)padding, padding_bytes);
    }

    // Close the stream and return true
    stream.close();
    return true;
}

//***************************************************************************************************//
//                                End of proffessor provided code                                    //
//***************************************************************************************************//


vector<vector<Pixel>> process_1(const vector<vector<Pixel>>& image)
{
    int num_rows = image.size();
    int num_columns = image[0].size();
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            int red_color = image[row][col].red;
            int green_color = image[row][col].green; 
            int blue_color = image[row][col].blue;
            
            double distance = sqrt(pow(col-num_columns/2,2) + pow(row-num_rows/2,2));
            double scaling_factor = (num_rows-distance)/num_rows;
            new_image[row][col].red = red_color * scaling_factor;
            new_image[row][col].green = green_color * scaling_factor;
            new_image[row][col].blue = blue_color * scaling_factor;
        }
    }
    return new_image;
}

vector<vector<Pixel>> process_2(const vector<vector<Pixel>>& image, double scaling_factor)
{
    int num_rows = image.size();
    int num_columns = image[0].size();
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            int red_color = image[row][col].red;
            int green_color = image[row][col].green; 
            int blue_color = image[row][col].blue;
            
            
            int average_value = (red_color + green_color + blue_color)/3;
            
            if (average_value >= 170)
            {
                new_image[row][col].red = 255 - (255 - red_color)*scaling_factor;
                new_image[row][col].green = 255 - (255 - green_color)*scaling_factor;
                new_image[row][col].blue = 255 - (255 - blue_color)*scaling_factor;
                
            }
            else if (average_value < 90)
            {
                new_image[row][col].red = red_color*scaling_factor;
                new_image[row][col].green = green_color*scaling_factor;
                new_image[row][col].blue = blue_color*scaling_factor;
            }
            else
            {
                new_image[row][col].red = red_color;
                new_image[row][col].green = green_color;
                new_image[row][col].blue = blue_color;
            }
             
        }
    }
    return new_image;
}

vector<vector<Pixel>> process_3(const vector<vector<Pixel>>& image)
{
    int num_rows = image.size();
    int num_columns = image[0].size();
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            int red_color = image[row][col].red;
            int green_color = image[row][col].green; 
            int blue_color = image[row][col].blue;
            
            double gray_value = (red_color + green_color + blue_color)/3;
            new_image[row][col].red = gray_value;
            new_image[row][col].green = gray_value;
            new_image[row][col].blue = gray_value;
        }
    }
    return new_image;
}


vector<vector<Pixel>> process_4(const vector<vector<Pixel>>& image)
{
    int num_rows = image.size();
    int num_columns = image[0].size();
    vector<vector<Pixel>> new_image(num_columns, vector<Pixel> (num_rows));
    
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            int red_color = image[row][col].red;
            int green_color = image[row][col].green; 
            int blue_color = image[row][col].blue;
            
            new_image[col][(num_rows) - row - 1].red = red_color;
            new_image[col][(num_rows) - row - 1].green = green_color;
            new_image[col][(num_rows) - row - 1].blue = blue_color;
            
        }
    }
    return new_image;
}

vector<vector<Pixel>> rotate_180(const vector<vector<Pixel>>& image)
{
    int num_rows = image.size();
    int num_columns = image[0].size();
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            int red_color = image[row][col].red;
            int green_color = image[row][col].green; 
            int blue_color = image[row][col].blue;
            
            new_image[num_rows - row - 1][(num_columns) - col - 1].red = red_color;
            new_image[num_rows - row - 1][(num_columns) - col - 1].green = green_color;
            new_image[num_rows - row - 1][(num_columns) - col - 1].blue = blue_color;
            
        }
    }
    return new_image;
}
    
vector<vector<Pixel>> rotate_270(const vector<vector<Pixel>>& image)
{
    int num_rows = image.size();
    int num_columns = image[0].size();
    vector<vector<Pixel>> new_image(num_columns, vector<Pixel> (num_rows));
    
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            int red_color = image[row][col].red;
            int green_color = image[row][col].green; 
            int blue_color = image[row][col].blue;
            
            new_image[col][row].red = red_color;
            new_image[col][row].green = green_color;
            new_image[col][row].blue = blue_color;
            
        }
    }
    return new_image;
}

vector<vector<Pixel>> process_5(const vector<vector<Pixel>>& image, int number)
{
    int angle = number * 90;
    if (angle%90 != 0)
    {
        cout << "Angle needs to be divisible by 90" << endl;
    }
    else if (angle%360 == 0)
    {
        return image;
    }
    else if (angle%360 == 90)
    {
        return process_4(image);
    }
    else if (angle%360 == 180)
    {
        return rotate_180(image);
    }
    else
    {
        return rotate_270(image); 
    }
        
    return image;
}

vector<vector<Pixel>> process_6(const vector<vector<Pixel>>& image, int xscale, int yscale)
{
    int num_rows = (image.size());
    int num_columns = (image[0].size());
    int y = num_rows* yscale;
    int x = num_columns* xscale;
    vector<vector<Pixel>> new_image(y, vector<Pixel> (x));
    for (int row = 0; row < y; row++)
    {
        for (int col = 0; col < x; col++)
        {
            int r = row/yscale;
            int c = col/xscale;
            int red_color = image[r][c].red;
            int green_color = image[r][c].green; 
            int blue_color = image[r][c].blue;
            
            new_image[row][col].red = red_color;
            new_image[row][col].green = green_color;
            new_image[row][col].blue = blue_color;
            
        }
    }
    return new_image;
}

vector<vector<Pixel>> process_7(const vector<vector<Pixel>>& image)
{
    int num_rows = image.size();
    int num_columns = image[0].size();
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            int red_color = image[row][col].red;
            int green_color = image[row][col].green; 
            int blue_color = image[row][col].blue;
            
            double gray_value = (red_color + green_color + blue_color)/3;
            
            if (gray_value >= 255/2)
            {
                new_image[row][col].red = 255;
                new_image[row][col].green = 255;
                new_image[row][col].blue = 255;
            }
            else
            {
                new_image[row][col].red = 0;
                new_image[row][col].green = 0;
                new_image[row][col].blue = 0;
            }
            
        }
    }
    return new_image;
}

vector<vector<Pixel>> process_8(const vector<vector<Pixel>>& image, double scaling_factor)
{
    int num_rows = image.size();
    int num_columns = image[0].size();
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            int red_color = image[row][col].red;
            int green_color = image[row][col].green; 
            int blue_color = image[row][col].blue;
            
            
            new_image[row][col].red = 255- (255 - red_color)*scaling_factor;
            new_image[row][col].green = 255- (255 - green_color)*scaling_factor;
            new_image[row][col].blue = 255- (255 - blue_color)*scaling_factor;
        }
    }
    return new_image;
}

vector<vector<Pixel>> process_9(const vector<vector<Pixel>>& image, double scaling_factor)
{
    int num_rows = image.size();
    int num_columns = image[0].size();
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            int red_color = image[row][col].red;
            int green_color = image[row][col].green; 
            int blue_color = image[row][col].blue;
            
            
            new_image[row][col].red = red_color*scaling_factor;
            new_image[row][col].green = green_color*scaling_factor;
            new_image[row][col].blue = blue_color*scaling_factor;
        }
    }
    return new_image;
}

vector<vector<Pixel>> process_10(const vector<vector<Pixel>>& image)
{
    int num_rows = image.size();
    int num_columns = image[0].size();
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            int red_color = image[row][col].red;
            int green_color = image[row][col].green; 
            int blue_color = image[row][col].blue;
            
            int max_a = max(red_color, green_color);
            int max_color = max(max_a, blue_color);
                
            if (red_color + green_color + blue_color >= 550)
            {
                new_image[row][col].red = 255;
                new_image[row][col].green = 255;
                new_image[row][col].blue = 255;
            }
            
            else if (red_color + green_color + blue_color <= 150)
            {
                new_image[row][col].red = 0;
                new_image[row][col].green = 0;
                new_image[row][col].blue = 0;
            }
            
            else if (max_color == red_color)
            {
                new_image[row][col].red = 255;
                new_image[row][col].green = 0;
                new_image[row][col].blue = 0;
            }  
            
            else if (max_color == green_color)
            {
                new_image[row][col].red = 0;
                new_image[row][col].green = 255;
                new_image[row][col].blue = 0;
            }  
            
            else
            {
                new_image[row][col].red = 0;
                new_image[row][col].green = 0;
                new_image[row][col].blue = 255;
            }
        }
    }
    return new_image;
}

string menu()
{
    
    cout << "IMAGE PROCESSING MENU" << endl;
    cout << "A) Change Image" << endl;
    cout << "B) Vignette" << endl;
    cout << "C) Clarendon" << endl;
    cout << "D) Grayscale" << endl;
    cout << "E) Rotate 90 degrees" << endl;
    cout << "F) Rotate 90 degree increment of choice" << endl;
    cout << "G) Enlarge by scale of choice" << endl;
    cout << "H) High contrast" << endl;
    cout << "I) Lighten" << endl;
    cout << "J) Darken" << endl;
    cout << "K) Black, white, red, green and blue only" << endl;
    
    cout << endl;
    cout << "Enter Menu Selection (Q to quit): ";
    string value;
    cin >> value;
    return value;
}


int main()
{

    cout << endl;
    cout << "CSPB 1300 Image Processing Application" << endl;
    cout << "Select an image" << endl;
    cout << endl;
    cout << "Enter input BMP filename: ";
    string filename;
    cin >> filename;
    int n = filename.length();
    while (filename.substr(n-4,4) != ".bmp")
    {
        cout << endl;
        cout << "Error, please enter a name that ends in .bmp: ";
        filename;
        cin >> filename;
        n = filename.length();
    }
            
    cout << endl;
    cout << "Filename is " << filename << endl;
    string input_filename = filename;
            cout << endl;
    
    string value = menu();
    
    while(value != "Q")
    {
        if (value == "A")
        {
            cout << "Select an image" << endl;
            cout << "Enter input BMP filename: ";
            string filename;
            cin >> filename;
            int n = filename.length();
            while (filename.substr(n-4,4) != ".bmp" || filename == input_filename)
            {
                cout << endl;
                cout << "Error, please enter a new name that ends in .bmp: ";
                filename;
                cin >> filename;
                n = filename.length();
            }
            
            cout << endl;
            cout << "Success! Your new filename is: " << filename << endl;
            cout << endl;
            input_filename = filename;
            value = menu();
        }
        
        else if(value == "B")
        {
            cout << input_filename << endl;
            cout << endl;
            vector<vector<Pixel>> image = read_image(input_filename);
            vector<vector<Pixel>> new_image = process_1(image);
            
            cout << "Success! The process worked and the image was created. Add in save name below." << endl;
            
            cout << endl;
            
            cout << "Enter your new BMP save filename: ";
            string new_filename;
            cin >> new_filename;
            int n2 = new_filename.length();
            while (new_filename.substr(n2-4,4) != ".bmp" || new_filename == input_filename)
            {
                cout << endl;
                cout << "Error, please enter a name that ends in .bmp: ";
                new_filename;
                cin >> new_filename;
                n2 = new_filename.length();
            }
            
            cout << endl;
            bool success = write_image(new_filename, new_image);
            cout << endl; 
            cout << "Success! A new file called " << new_filename << " has been created!" << endl;
            cout << endl;
            value = menu();
        }
        
        else if(value == "C")
        {
            cout << input_filename << endl;
            cout << endl;
            vector<vector<Pixel>> image = read_image(input_filename);
            
            
            
            double clarendon_scale;
            cout << "Enter a clarendon scale factor between 0 and 1: ";
            cin >> clarendon_scale;
            if(cin.fail())
            {
                cout << endl;
                cout << "Error, invalid input type. Start over and try again." << endl;
                cout << endl;
                return 1;
            }
            
            while (clarendon_scale >= 1 || clarendon_scale <= 0)
            {
                cout << endl;
                cout << "Error, please enter a decimal value between 0 and 1 ";
                clarendon_scale;
                cin >> clarendon_scale;
                
                if(cin.fail())
                {
                cout << endl;
                cout << "Error, invalid input type. Start over and try again." << endl;
                cout << endl;
                return 1;
                }
            }
            
            vector<vector<Pixel>> new_image = process_2(image, clarendon_scale);
            cout << endl;
            cout << "Success! The process worked and the image was created. Add in save name below." << endl;
            
            cout << endl;
            
            cout << "Enter your new BMP save filename: ";
            string new_filename;
            cin >> new_filename;
            int n2 = new_filename.length();
            while (new_filename.substr(n2-4,4) != ".bmp" || new_filename == input_filename)
            {
                cout << endl;
                cout << "Error, please enter a name that ends in .bmp: ";
                new_filename;
                cin >> new_filename;
                n2 = new_filename.length();
            }
            
            cout << endl;
            
            bool success = write_image(new_filename, new_image);
            cout << endl; 
            cout << "Success! A new file called " << new_filename << " has been created!" << endl;
            cout << endl;
            value = menu();
        }
        
        else if(value == "D")
        {
            cout << input_filename << endl;
            cout << endl;
            vector<vector<Pixel>> image = read_image(input_filename);
            vector<vector<Pixel>> new_image = process_3(image);
            
            cout << "Success! The process worked and the image was created. Add in save name below." << endl;
            
            cout << endl;
            
            cout << "Enter your new BMP save filename: ";
            string new_filename;
            cin >> new_filename;
            int n2 = new_filename.length();
            while (new_filename.substr(n2-4,4) != ".bmp" || new_filename == input_filename)
            {
                cout << endl;
                cout << "Error, please enter a name that ends in .bmp: ";
                new_filename;
                cin >> new_filename;
                n2 = new_filename.length();
            }
            cout << endl;
            
            bool success = write_image(new_filename, new_image);
            cout << endl;
            cout << "Success! A new file called " << new_filename << " has been created!" << endl;
            cout << endl;
            value = menu();
        }
        
        else if(value == "E")
        {
            cout << input_filename << endl;
            cout << endl;
            vector<vector<Pixel>> image = read_image(input_filename);  
            vector<vector<Pixel>> new_image = process_4(image);
            
            cout << "Success! The process worked and the image was created. Add in save name below." << endl;
            
            cout << endl;
            
            cout << "Enter your new BMP save filename: ";
            string new_filename;
            cin >> new_filename;
            int n2 = new_filename.length();
            while (new_filename.substr(n2-4,4) != ".bmp" || new_filename == input_filename)
            {
                cout << endl;
                cout << "Error, please enter a name that ends in .bmp: ";
                new_filename;
                cin >> new_filename;
                n2 = new_filename.length();
            }
            cout << endl;
            
            bool success = write_image(new_filename, new_image);
            cout << endl;  
            cout << "Success! A new file called " << new_filename << " has been created!" << endl;
            cout << endl;
            value = menu();
        }
        
        else if(value == "F")
        {
            cout << input_filename << endl;
            cout << endl;
            vector<vector<Pixel>> image = read_image(input_filename);
            
            cout << "Enter the number of clockwise rotations between 1 and 100: ";
            int rotations;
            cin >> rotations;
            if(cin.fail())
            {
                cout << endl;
                cout << "Error, invalid input type. Start over and try again." << endl;
                cout << endl;
                return 1;
            }
            while (rotations > 100 || rotations < 1)
            {
                cout << endl;
                cout << "Error, please enter a whole number between 1 and 100 ";
                rotations;
                cin >> rotations;
                if(cin.fail())
                {
                cout << endl;
                cout << "Error, invalid input type. Start over and try again." << endl;
                cout << endl;
                return 1;
                }
            }
            cout << endl;
            
            vector<vector<Pixel>> new_image = process_5(image, rotations);
            
            cout << "Success! The process worked and the image was created. Add in save name below." << endl;
            
            cout << endl;
            
            cout << "Enter your new BMP save filename: ";
            string new_filename;
            cin >> new_filename;
            int n2 = new_filename.length();
            while (new_filename.substr(n2-4,4) != ".bmp" || new_filename == input_filename)
            {
                cout << endl;
                cout << "Error, please enter a name that ends in .bmp: ";
                new_filename;
                cin >> new_filename;
                n2 = new_filename.length();
            }
            cout << endl;
            
            bool success = write_image(new_filename, new_image);
            cout << endl;
            cout << "Success! A new file called " << new_filename << " has been created!" << endl;
            cout << endl;
            value = menu();
        }
        
        else if(value == "G")
        {
            cout << input_filename << endl;
            cout << endl;
            vector<vector<Pixel>> image = read_image(input_filename);
            cout << "Enter a xscale value between 2 and 5: ";
            int x;
            cin >> x;
            if(cin.fail())
            {
                cout << endl;
                cout << "Error, invalid input type. Start over and try again." << endl;
                cout << endl;
                return 1;
            }
            while (x > 5 || x < 2)
            {
                cout << endl;
                cout << "Error, please enter a whole number between 2 and 5 ";
                x;
                cin >> x;
                if(cin.fail())
                {
                cout << endl;
                cout << "Error, invalid input type. Start over and try again." << endl;
                cout << endl;
                return 1;
                }
            }
            
            cout << "Enter a yscale value between 2 and 5: ";
            int y;
            cin >> y;
            if(cin.fail())
            {
                cout << endl;
                cout << "Error, invalid input type. Start over and try again." << endl;
                cout << endl;
                return 1;
            }
            while (y > 5 || y < 2)
            {
                cout << endl;
                cout << "Error, please enter a whole number between 2 and 5 ";
                y;
                cin >> y;
                if(cin.fail())
                {
                cout << endl;
                cout << "Error, invalid input type. Start over and try again." << endl;
                cout << endl;
                return 1;
                }
            }
            
            cout << endl;
            
            vector<vector<Pixel>> new_image = process_6(image, x, y);
            
            cout << "Success! The process worked and the image was created. Add in save name below." << endl;
            
            cout << endl;
            
            cout << "Enter your new BMP save filename: ";
            string new_filename;
            cin >> new_filename;
            int n2 = new_filename.length();
            while (new_filename.substr(n2-4,4) != ".bmp" || new_filename == input_filename)
            {
                cout << endl;
                cout << "Error, please enter a name that ends in .bmp: ";
                new_filename;
                cin >> new_filename;
                n2 = new_filename.length();
            }
            cout << endl;
            
            bool success = write_image(new_filename, new_image);
            cout << endl;
            cout << "Success! A new file called " << new_filename << " has been created!" << endl;
            cout << endl;
            value = menu();
        }
        
        else if(value == "H")
        {
            cout << input_filename << endl;
            cout << endl;
            vector<vector<Pixel>> image = read_image(input_filename);
            vector<vector<Pixel>> new_image = process_7(image);
            
            cout << "Success! The process worked and the image was created. Add in save name below." << endl;
            
            cout << endl;
            
            cout << "Enter your new BMP save filename: ";
            string new_filename;
            cin >> new_filename;
            int n2 = new_filename.length();
            while (new_filename.substr(n2-4,4) != ".bmp" || new_filename == input_filename)
            {
                cout << endl;
                cout << "Error, please enter a name that ends in .bmp: ";
                new_filename;
                cin >> new_filename;
                n2 = new_filename.length();
            }
            cout << endl;
            
            bool success = write_image(new_filename, new_image);
            cout << endl;
            cout << "Success! A new file called " << new_filename << " has been created!" << endl;
            cout << endl;
            value = menu();
        }
        
        else if(value == "I")
        {
            cout << input_filename << endl;
            cout << endl;
            vector<vector<Pixel>> image = read_image(input_filename);
            
            cout << "Enter a decimal value for lightening scaling value between 0 and 1: ";
            double lighten_factor;
            cin >> lighten_factor;
            if(cin.fail())
            {
                cout << endl;
                cout << "Error, invalid input type. Start over and try again." << endl;
                cout << endl;
                return 1;
            }
            while (lighten_factor <= 0 || lighten_factor >= 1)
            {
                cout << endl;
                cout << "Error, please enter a decimal value between 0 and 1: ";
                lighten_factor;
                cin >> lighten_factor;
                if(cin.fail())
                {
                cout << endl;
                cout << "Error, invalid input type. Start over and try again." << endl;
                cout << endl;
                return 1;
                }
            }
            cout << endl;
            
            vector<vector<Pixel>> new_image = process_8(image, lighten_factor);
            
            cout << "Success! The process worked and the image was created. Add in save name below." << endl;
            
            cout << endl;
            
            cout << "Enter your new BMP save filename: ";
            string new_filename;
            cin >> new_filename;
            int n2 = new_filename.length();
            while (new_filename.substr(n2-4,4) != ".bmp" || new_filename == input_filename)
            {
                cout << endl;
                cout << "Error, please enter a name that ends in .bmp: ";
                new_filename;
                cin >> new_filename;
                n2 = new_filename.length();
            }
            cout << endl;
            
            bool success = write_image(new_filename, new_image);
            cout << endl;
            cout << "Success! A new file called " << new_filename << " has been created!" << endl;
            cout << endl;
            value = menu();
        }

        else if(value == "J")
        {
            cout << input_filename << endl;
            cout << endl;
            vector<vector<Pixel>> image = read_image(input_filename);
            
            cout << "Enter a decimal value for darkening scaling value between 0 and 1: ";
            double darken_factor;
            cin >> darken_factor;
            if(cin.fail())
            {
                cout << endl;
                cout << "Error, invalid input type. Start over and try again." << endl;
                cout << endl;
                return 1;
            }
            while (darken_factor <= 0 || darken_factor >= 1)
            {
                cout << endl;
                cout << "Error, please enter a decimal value between 0 and 1: ";
                darken_factor;
                cin >> darken_factor;
                if(cin.fail())
                {
                cout << endl;
                cout << "Error, invalid input type. Start over and try again." << endl;
                cout << endl;
                return 1;
                }
            }
            cout << endl;
            
            vector<vector<Pixel>> new_image = process_9(image, darken_factor);
            
            cout << "Success! The process worked and the image was created. Add in save name below." << endl;
            
            cout << endl;
            
            cout << "Enter your new BMP save filename: ";
            string new_filename;
            cin >> new_filename;
            int n2 = new_filename.length();
            while (new_filename.substr(n2-4,4) != ".bmp" || new_filename == input_filename)
            {
                cout << endl;
                cout << "Error, please enter a name that ends in .bmp: ";
                new_filename;
                cin >> new_filename;
                n2 = new_filename.length();
            }
            cout << endl;
            
            bool success = write_image(new_filename, new_image);
            cout << endl; 
            cout << "Success! A new file called " << new_filename << " has been created!" << endl;
            cout << endl;
            value = menu();
        }
        
        else if(value == "K")
        {
            cout << input_filename << endl;
            cout << endl;
            vector<vector<Pixel>> image = read_image(input_filename);
            vector<vector<Pixel>> new_image = process_10(image);
            
            cout << "Success! The process worked and the image was created. Add in save name below." << endl;
            
            cout << endl;
            
            cout << "Enter your new BMP save filename: ";
            string new_filename;
            cin >> new_filename;
            int n2 = new_filename.length();
            while (new_filename.substr(n2-4,4) != ".bmp" || new_filename == input_filename)
            {
                cout << endl;
                cout << "Error, please enter a name that ends in .bmp: ";
                new_filename;
                cin >> new_filename;
                n2 = new_filename.length();
            }
            
            cout << endl;
            bool success = write_image(new_filename, new_image);
            cout << endl; 
            cout << "Success! A new file called " << new_filename << " has been created!" << endl;
            cout << endl;
            value = menu();
        }
        
        else
        {
            cout << endl;
            cout << "Please enter a valid selection" << endl;
            cout << endl;
            value = menu();
        }
    }
    cout << endl;
    cout << "Goodbye" <<endl;
    cout << endl;

    return 0;
}

