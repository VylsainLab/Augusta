#ifndef _UTILS
#define _UTILS

#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace Geodetics
{

	static double dFlattening = 1.0 / 298.257222101;
	static double dEquatorialRadius = 6378137.;
	static double dExentricity = dFlattening * (2. - dFlattening);

	static void ECEFtoPolar(double x, double y, double z, double& lat, double& lon, double& alt)
	{
		if (x == 0. && y == 0. && z == 0.)
		{
			lat = 0.;
			lon = 0.;
			alt = 0.;
			return;
		}

		double sinLat;  // cache for sin of latitude
		double cosLat;  // cache for cos of latitude
		double curvature;  // latitude dependent curvature
		const double radiusXY = sqrt(x * x + y * y);  // (invariant)
		const double opp = z;  // opposite length of latitude (invariant)
		double adj;  // adjacent length of latitude
		double hypInv;  // inverse hypotenuse length of latitude

		//double lat, lon, alt;

		// compute longitude exactly
		lon = atan2(y, x);
		// occasionally this is returning a nan value.  A second call with the same parameters works correctly.
		// This is tracked by bug #197219.
		if (_isnan(lon))
		{
			lon = atan2(y, x);
			if (_isnan(lon))
			{
				// if this fails a second time return 0 for all values.
				lat = 0;
				lon = 0;
				alt = 0;
				return;
			}
		}

		// compute initial guess for latitude and altitude
		adj = radiusXY * (1. - dFlattening);
		hypInv = 1.0 / sqrt(opp * opp + adj * adj);

		lat = atan2(opp, adj);

		sinLat = opp * hypInv;  // sinLat = System.Math.Sin(latitude);
		cosLat = adj * hypInv;

		curvature = dEquatorialRadius;  // initial sphere guess works best: curvature = sRadius / sqrt(1.0 - getEccentricity2() * sinLat *
		// sinLat);
		if (radiusXY > 1.0)
		{
			alt = radiusXY / cosLat - curvature;
		}
		else if (z > 0.0)
		{
			alt = z - dEquatorialRadius * (1.0 - dFlattening);
		}
		else
		{
			alt = -z - dEquatorialRadius * (1.0 - dFlattening);
		}

		// first iteration
		adj = radiusXY * (1.0 - dExentricity * curvature / (curvature + alt));
		hypInv = 1.0 / sqrt(opp * opp + adj * adj);

		lat = atan2(opp, adj);

		sinLat = opp * hypInv;  // sinLat = System.Math.Sin(latitude);
		cosLat = adj * hypInv;

		curvature = dEquatorialRadius / sqrt(1.0 - dExentricity * sinLat * sinLat);
		if (radiusXY > 1.0)
		{
			alt = radiusXY / cosLat - curvature;
		}
		else if (z > 0.0)
		{
			alt = z - dEquatorialRadius * (1.0 - dFlattening);
		}
		else
		{
			alt = -z - dEquatorialRadius * (1.0 - dFlattening);
		}

		// second iteration
		adj = radiusXY * (1.0 - dExentricity * curvature / (curvature + alt));
		hypInv = 1.0 / sqrt(opp * opp + adj * adj);

		lat = atan2(opp, adj);

		sinLat = opp * hypInv;  // sinLat = System.Math.Sin(latitude);
		cosLat = adj * hypInv;

		curvature = dEquatorialRadius / sqrt(1.0 - dExentricity * sinLat * sinLat);
		if (radiusXY > 1.0)
		{
			alt = radiusXY / cosLat - curvature;
		}
		else if (z > 0.0)
		{
			alt = z - dEquatorialRadius * (1.0 - dFlattening);
		}
		else
		{
			alt = -z - dEquatorialRadius * (1.0 - dFlattening);
		}

		// third iteration
		adj = radiusXY * (1.0 - dExentricity * curvature / (curvature + alt));
		hypInv = 1.0 / sqrt(opp * opp + adj * adj);

		lat = atan2(opp, adj);

		sinLat = opp * hypInv;  // sinLat = System.Math.Sin(latitude);
		cosLat = adj * hypInv;

		curvature = dEquatorialRadius / sqrt(1.0 - dExentricity * sinLat * sinLat);
		if (radiusXY > 1.0)
		{
			alt = radiusXY / cosLat - curvature;
		}
		else if (z > 0.0)
		{
			alt = z - dEquatorialRadius * (1.0 - dFlattening);
		}
		else
		{
			alt = -z - dEquatorialRadius * (1.0 - dFlattening);
		}

		lon = fmod(lon + M_PI, 2. * M_PI) - M_PI;

		lat = lat * 180. / M_PI;
		lon = lon * 180. / M_PI;
	}

	inline double DegToRad(double dDeg)
	{
		return dDeg * M_PI / 180.;
	}

	inline double RadToDeg(double dRad)
	{
		return dRad * 180. / M_PI;
	}

	inline glm::dmat4 BuildMatrixFromAngles(double theta, double phi, double psi)
	{
		glm::dmat4 M = glm::dmat4(1.0f);
		M = glm::rotate(M, psi, glm::dvec3(0, 0, 1)); // Yaw
		M = glm::rotate(M, theta, glm::dvec3(0, 1, 0)); // Pitch
		M = glm::rotate(M, phi, glm::dvec3(1, 0, 0)); // Roll

		return M;
	}

	inline void GetAnglesFromMatrix(glm::dmat4 m, double& theta, double& phi, double& psi)
	{
		theta = asin(-m[2][0]);
		phi = atan2(m[2][1], m[2][2]);
		psi = atan2(m[1][0], m[0][0]);
	}

	//https://gssc.esa.int/navipedia/index.php/Transformations_between_ECEF_and_ENU_coordinates
	inline glm::dmat4 ECEFtoENU(double dLat, double dLon)
	{
		return glm::dmat4(
			-sin(dLon), cos(dLon), 0, 0,	//east
			-cos(dLon) * sin(dLat), -sin(dLon) * sin(dLat), cos(dLat), 0,	//north
			cos(dLon) * cos(dLat), sin(dLon) * cos(dLat), sin(dLat), 0,	//up
			0, 0, 0, 1
		);
	}

	inline glm::dmat4 ECEFtoNED(double dLat, double dLon)
	{
		return glm::dmat4(
			-cos(dLon) * sin(dLat), -sin(dLon) * sin(dLat), cos(dLat), 0,	//north
			-sin(dLon), cos(dLon), 0, 0,	//east			
			-cos(dLon) * cos(dLat), -sin(dLon) * cos(dLat), -sin(dLat), 0,	//down
			0, 0, 0, 1
		);
	}

	inline glm::dmat4 ENUtoECEF(double dLat, double dLon)
	{
		return glm::dmat4(
			-sin(dLon), -cos(dLon) * sin(dLat), cos(dLon) * cos(dLat), 0,	//x
			cos(dLon), -sin(dLon) * sin(dLat), sin(dLon) * cos(dLat), 0,	//y
			0, cos(dLat), sin(dLat), 0,	//z
			0, 0, 0, 1
		);
	}

}

#endif