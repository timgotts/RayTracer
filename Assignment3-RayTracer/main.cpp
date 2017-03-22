#include "Scene.h"
#include "InputReader.h"
#include "Sphere.h"
#include <cimg/CImg.h>
#include "Plane.h"
#define MAX_RAY_DEPTH = 5;

const float ambient_light = 0.7;

float saturate(float x)
{
	if (x > 255.f)
	{
		return 255.f;
	}
	if (x < 0)
	{
		return 0.f;
	}
	return x;
}

void writeImage(int width, int height, glm::vec3* pixels)
{
	cimg_library::CImg<float> image(width, height, 1, 3, 0);
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			int index = y * width + x;
			float red = pixels[index].x * 255;
			float green = pixels[index].y * 255;
			float blue = pixels[index].z * 255;
			image(x, height - y - 1, 0, 0) = red;
			image(x, height - y - 1, 0, 1) = green;
			image(x, height - y - 1, 0, 2) = blue;
		}
	}
	image.normalize(0, 255);
	image.save("render.bmp");
	cimg_library::CImgDisplay main_disp(image, "Render");
	while (!main_disp.is_closed())
	{
		main_disp.wait();
	}
}

int closest(const std::vector<float>& intersections)
{
	int index_min;
	// Prevent unneeded calculations
	if (intersections.size() == 0) // No intersections were found
	{
		return -1;
	}

	if (intersections.size() == 1) // Only one intersection found
	{
		if (intersections[0] > 0) // Make sure the distance is greater than 0
		{
			return 0;
		}
		else
		{
			return -1; // The only intersection distance is negative
		}
	}
	// Determine the closest intersection to camera
	double max = 0;
	for (int i = 0; i < intersections.size(); i++)
	{
		if (intersections[i] > max) max = intersections[i];
	}
	// Starting from the maximum value, find the minimum positive value
	if (max > 0)
	{
		for (int i = 0; i < intersections.size(); i++)
		{
			if (intersections[i] > 0 && intersections[i] <= max)
			{
				max = intersections.at(i);
				index_min = i;
			}
		}
		return index_min;
	}
	return -1;

}


//glm::vec3 Trace(Ray& ray, std::vector<SceneObject*> objects, std::vector<Light*> lights,const int&depth)
//{
//	SceneObject *object = nullptr;
//	float minDist = INFINITY;
//	glm::vec3 pHit;
//	glm::vec3 nHit;
//	for (int k=0; k<objects.size(); k++)
//	{
//		float distance = objects.at(k)->Intersect(ray);
//		if (distance > 0 && distance<minDist)
//		{
//			object = objects.at(k);
//			minDist = distance;
//		}
//		if (object==nullptr)
//		{
//			// No intersection, return background color
//			return glm::vec3(0, 0, 0);
//		}
//		
//
//	}
//}

glm::vec3 getColorAt(const glm::vec3& intersection_position, const glm::vec3& intersection_direction, const std::vector<SceneObject*>& objects, const std::vector<Light*>& lights, int index, double accuracy)
{
	//Ambient color
	glm::vec3 color = ambient_light*objects.at(index)->getAmbient();
	//std::cout << "ambient:" << color.x << " " << color.y << " " << color.z << std::endl;

	glm::vec3 surface_diffuse = objects.at(index)->getDiffuse();
	glm::vec3 surface_specular = objects.at(index)->getSpecular();
	float surface_shininess = objects.at(index)->getShininess();
	glm::vec3 object_normal = normalize(objects.at(index)->getNormalAt(intersection_position));


	glm::vec3 final_color = color;

	for (int light_index = 0; light_index < lights.size(); light_index++)
	{
		glm::vec3 light_direction = normalize(lights.at(light_index)->getPosition() - intersection_position);
		float cosine_angle = dot(object_normal, light_direction);
		if (cosine_angle > 0)
		{
			// Check for shadows
			bool shadowed = false;
			float distance_to_light = length(lights.at(light_index)->getPosition() - intersection_position);
			glm::vec3 shadow_ray_origin = intersection_position + light_direction*(float)0.1;
			Ray shadow_ray(shadow_ray_origin, light_direction);
			std::vector<float> secondary_intersections;
			for (int object_index = 0; object_index < objects.size(); object_index++)
			{
				secondary_intersections.push_back(objects.at(object_index)->Intersect(shadow_ray));
			}

			for (int c = 0; c < secondary_intersections.size(); c++)
			{
				if (secondary_intersections.at(c) > accuracy)
				{
					if (secondary_intersections.at(c) <= distance_to_light)
					{
						shadowed = true;
						break;
					}
				}
			}
			if (!shadowed) {
				glm::vec3 addedLight = glm::vec3(0, 0, 0);

				glm::vec3 diffuse = surface_diffuse * std::max((float)0, cosine_angle);

				glm::vec3 r = glm::normalize(2 * dot(light_direction, object_normal) * object_normal - light_direction);
				float rDotL = dot(r, light_direction);
				if (rDotL > accuracy) {
					glm::vec3 specular = surface_specular * std::max((float)0, pow(rDotL, surface_shininess));

					//addedLight += lights.at(light_index)->getColor()*(diffuse + specular);
					addedLight.x += lights.at(light_index)->getColor().x*(diffuse.x + specular.x);
					addedLight.y += lights.at(light_index)->getColor().y*(diffuse.y + specular.x);
					addedLight.z += lights.at(light_index)->getColor().z*(diffuse.z + specular.z);
				}
				else
				{
					//addedLight += lights.at(light_index)->getColor()*(diffuse);
					addedLight.x += lights.at(light_index)->getColor().x*diffuse.x;
					addedLight.y += lights.at(light_index)->getColor().y*diffuse.y;
					addedLight.z += lights.at(light_index)->getColor().z*diffuse.z;
				}
				final_color += addedLight;
			}
		}

	}
	return final_color;
}

int main()
{
	double accuracy = 0.00001;

	glm::vec3 cam_dir = glm::vec3(0, 0, -1);
	glm::vec3 cam_down = glm::vec3(0, -1, 0);
	glm::vec3 cam_right = glm::vec3(1, 0, 0);

	InputReader* input = new InputReader("scene7.txt");

	Scene scene = input->scene;

	int width = scene.width;
	int height = scene.height;
	int numPixels = width * height;
	glm::vec3* pixels = new glm::vec3[numPixels];

	float aspect_ratio = scene.camera->getAspectRatio();
	glm::vec3 Cam_ray_origin = scene.camera->getPosition();
	float fov = glm::radians(scene.camera->getFoV());


	float xamt, yamt;
	glm::vec3 pHit;
	glm::vec3 nHit;
	glm::vec3 dHit;

	float minDistance = INFINITY;

	SceneObject* object;
	//float tanFoVdiv2 = tan(fov / 2);
	////result = (src - src_min) / (src_max - src_min) * (result_max - result_min) + result_min
	//float rayDivY = -height*(-tanFoVdiv2 -tanFoVdiv2) - tanFoVdiv2;
	//float rayDivX = -width *(aspect_ratio*tanFoVdiv2*2) + (-aspect_ratio * tanFoVdiv2);

	int index = 0;

	for (int x = 0; x < width; x++)
	{
		if (x % 10 == 0)
		{
			std::cout << "Tracing pixels for x=" << x << "... " << std::endl;
		}
		for (int y = 0; y < height; y++)
		{

			// Compute values for the ray direction
			if (width > height)
			{
				xamt = ((x + 0.5) / width)*aspect_ratio - (((width - height) / (float)height) / 2);
				yamt = ((height - y) + 0.5) / height;
			}
			else if (height > width)
			{
				xamt = (x + 0.5) / width;
				yamt = (((height - y) + 0.5) / height) / aspect_ratio - (((height - width) / (float)width) / 2);
			}
			else
			{
				// The image is square
				xamt = (x + 0.5) / width;
				yamt = ((height - y) + 0.5) / height;
			}


			//float rayY = (y-height) / rayDivY;
			//float rayX = (x-width) / rayDivX;
			//glm::vec3 Cam_ray_direction = glm::normalize(glm::vec3(rayX, rayY, -1.f)-Cam_ray_origin);
			glm::vec3 Cam_ray_direction = glm::normalize(cam_dir + (cam_right*(float)(xamt - 0.5) + (cam_down*(float)(yamt - 0.5))));

			Ray cam_ray(Cam_ray_origin, Cam_ray_direction);

			std::vector<float> intersections;

			for (int i = 0; i < scene.objects.size(); i++)
			{
				intersections.push_back(scene.objects[i]->Intersect(cam_ray));
			}

			int index_closest = closest(intersections);
			index = y*width + x;

			if (index_closest >= 0)
			{
				// Determine the position / direction vectors at intersection point
				if (intersections.at(index_closest) > accuracy)
				{
					glm::vec3 intersection_position = Cam_ray_origin + (Cam_ray_direction*intersections.at(index_closest));
					glm::vec3 intersection_direction = Cam_ray_direction;
					glm::vec3 intersection_color = getColorAt(intersection_position, intersection_direction, scene.objects, scene.lights, index_closest, accuracy);

					pixels[index].x = intersection_color.x;
					pixels[index].y = intersection_color.y;
					pixels[index].z = intersection_color.z;
				}
			}
			else
			{
				pixels[index].x = 0;
				pixels[index].y = 0;
				pixels[index].z = 0;
			}
		}
	}
	std::cout << "Reached end of raycast loop" << std::endl;
	writeImage(width, height, pixels);

	return 0;
}